/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file S3Context.cpp
 ** \~french
 * \brief Implémentation de la classe S3Context
 * \details
 * \li S3Context : connexion à un container S3
 ** \~english
 * \brief Implement classe S3Context
 * \details
 * \li S3Context : S3 container connection
 */

#include "storage/S3Context.h"

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <rok4/storage/Context.h>
#include <sys/stat.h>
#include <time.h>

#include "S3Context.h"
#include "utils/Cache.h"
#include "utils/LibcurlStruct.h"

std::vector<std::string> S3Context::env_hosts;
std::vector<std::string> S3Context::env_keys;
std::vector<std::string> S3Context::env_secret_keys;
std::vector<std::string> S3Context::env_cluster_names;
std::vector<std::string> S3Context::env_urls;
bool S3Context::ssl_no_verify = false;

bool S3Context::load_env() {
    if (!env_hosts.empty()) {
        return true;
    }

    std::string urls, keys, secret_keys;
    char *e;
    std::stringstream ss;
    std::string token, tmp;
    const char delim = ',';
    std::size_t pos;

    // Chargement des variables d'environnement et ajout des valeurs par défaut

    e = getenv(ROK4_S3_URL);
    if (e == NULL) {
        urls = "http://localhost:9000";
    } else {
        urls = std::string(e);
    }
    ss = std::stringstream(urls);
    while (std::getline(ss, token, delim)) {
        env_urls.push_back(token);

        // On enlève le protocole
        pos = token.find("://");
        if (pos != std::string::npos) {
            tmp = token.substr(pos + 3);
        } else {
            tmp = token;
        }

        // On a le nom du cluster
        env_cluster_names.push_back(tmp);

        // On enlève le port pour avoir l'hôte
        pos = tmp.find(":");
        if (pos != std::string::npos) {
            env_hosts.push_back(tmp.substr(0, pos));
        } else {
            env_hosts.push_back(tmp);
        }
    }

    e = getenv(ROK4_S3_KEY);
    if (e == NULL) {
        keys = "rok4";
    } else {
        keys = std::string(e);
    }
    ss = std::stringstream(keys);
    while (std::getline(ss, token, delim)) {
        env_keys.push_back(token);
    }

    e = getenv(ROK4_S3_SECRETKEY);
    if (e == NULL) {
        secret_keys = "rok4S3storage";
    } else {
        secret_keys = std::string(e);
    }
    ss = std::stringstream(secret_keys);
    while (std::getline(ss, token, delim)) {
        env_secret_keys.push_back(token);
    }

    if (getenv(ROK4_SSL_NO_VERIFY) != NULL) {
        ssl_no_verify = true;
    }

    // Analyse des valeurs

    if (env_urls.size() != env_keys.size() || env_urls.size() != env_secret_keys.size()) {
        BOOST_LOG_TRIVIAL(error) << "S3 informations in environment variables are inconsistent : same number of element in each list is required";
        // On vide les listes pour signaler l'erreur de chargement
        env_urls.clear();
        env_keys.clear();
        env_secret_keys.clear();
        return false;
    }

    return true;
}

std::string S3Context::get_default_cluster() {
    if (!load_env()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot load environment variables to use S3 storage";
        return "";
    }
    return env_cluster_names.at(0);
}

S3Context::S3Context(std::string b) : Context(), bucket_name(b), cluster_name("") {
    if (!load_env()) {
        BOOST_LOG_TRIVIAL(error) << "Cannot load environment variables to use S3 storage";
        return;
    }

    // Le cluster est il renseigné dans la clef ?
    // ex. bucket@cluster
    // Pour eviter les doublons, le port du cluster doit être renseigné !

    std::size_t pos = b.find("@");
    if (pos != std::string::npos) {
        this->cluster_name = b.substr(pos + 1);
        // On supprime le nom du cluster du nom du bucket
        this->bucket_name = b.substr(0, pos);
    } else {
        // Pas de nom de cluster dans le nom de bucket, on met celui par défaut, le premier
        this->cluster_name = env_cluster_names.at(0);
    }

    // On cherche l'index des informations S3 pour ce bucket
    int index = -1;
    for (int i = 0; i < env_cluster_names.size(); i++) {
        if (env_cluster_names.at(i) == this->cluster_name) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        BOOST_LOG_TRIVIAL(error) << "No S3 informations for S3 cluster '" << this->cluster_name << "'";
        // Le fait que le host soit vide sera utilisé pour empêcher la connection et l'utilisation de ce contexte S3
        return;
    }

    this->host = env_hosts.at(index);
    this->key = env_keys.at(index);
    this->secret_key = env_secret_keys.at(index);
    this->url = env_urls.at(index);
}

bool S3Context::connection() {
    if (host == "") {
        connected = false;
        return false;
    } else {
        connected = true;
        return true;
    }
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::string S3Context::getAuthorizationHeader(std::string toSign) {
    unsigned char bytes_to_encode[EVP_MAX_MD_SIZE];
    unsigned int bytes_to_encode_len;

    HMAC(EVP_sha1(), secret_key.c_str(), secret_key.length(), (const unsigned char *)toSign.c_str(), toSign.length(), bytes_to_encode, &bytes_to_encode_len);
    std::string signature = base64_encode(bytes_to_encode, bytes_to_encode_len);

    return signature;
}

/**
 * \~french \brief Noms court des jours en anglais
 * \~english \brief Short english day names
 */
static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

/**
 * \~french \brief Noms court des mois en anglais
 * \~english \brief Short english month names
 */
static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int S3Context::read(uint8_t *data, int offset, int size, std::string name) {

    BOOST_LOG_TRIVIAL(debug) << "S3 read : " << size << " bytes (from the " << offset << " one) in the object " << bucket_name << "@" << ((cluster_name != "") ? cluster_name : host) << " / " << name;

    int attempt = 1;
    while (attempt <= read_attempts) {
    // On constitue le moyen de récupération des informations (avec les structures de LibcurlStruct)

        CURLcode res;
        struct curl_slist *list = NULL;
        DataStruct chunk;
        chunk.nbPassage = 0;
        chunk.data = (char *)malloc(1);
        chunk.size = 0;

        int lastBytes = offset + size - 1;

        CURL *curl = CurlPool::get_curl_env();

        std::string fullUrl = url + "/" + bucket_name + "/" + name;

        time_t current;

        time(&current);
        struct tm *ptm = gmtime(&current);

        static char gmt_time[40];
        sprintf(
            gmt_time, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
            wday_name[ptm->tm_wday], ptm->tm_mday, mon_name[ptm->tm_mon], 1900 + ptm->tm_year,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

        std::string content_type = "application/octet-stream";
        std::string resource = "/" + bucket_name + "/" + name;
        std::string stringToSign = "GET\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
        std::string signature = getAuthorizationHeader(stringToSign);

        // Constitution du header

        char range[50];
        sprintf(range, "Range: bytes=%d-%d", offset, lastBytes);
        list = curl_slist_append(list, range);

        char hd_host[256];
        sprintf(hd_host, "Host: %s", host.c_str());
        list = curl_slist_append(list, hd_host);

        char d[100];
        sprintf(d, "Date: %s", gmt_time);
        list = curl_slist_append(list, d);

        char ct[50];
        sprintf(ct, "Content-Type: %s", content_type.c_str());
        list = curl_slist_append(list, ct);

        std::string ex = "Expect:";
        list = curl_slist_append(list, ex.c_str());

        char auth[512];
        sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());

        list = curl_slist_append(list, auth);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if (ssl_no_verify) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        BOOST_LOG_TRIVIAL(debug) << "S3 READ START (" << size << ") " << pthread_self();
        res = curl_easy_perform(curl);
        BOOST_LOG_TRIVIAL(debug) << "S3 READ END (" << size << ") " << pthread_self();

        curl_slist_free_all(list);
        // delete[] gmt_time;

        if (CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            attempt++;
            sleep(waiting_time);
            continue;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code < 200 || http_code > 299) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
            attempt++;
            sleep(waiting_time);
            continue;
        }

        memcpy(data, chunk.data, chunk.size);

        return chunk.size;
    }

    BOOST_LOG_TRIVIAL(error) <<  "Unable to read " << size << " bytes (from the " << offset << " one) from the S3 object " << bucket_name << "@" << ((cluster_name != "") ? cluster_name : host) << " / " << name << " after " << read_attempts << " tries" ;

    return -1;
}

uint8_t *S3Context::read_full(int &size, std::string name) {
    size = -1;

    BOOST_LOG_TRIVIAL(debug) << "S3 read full : " << bucket_name << "@" << ((cluster_name != "") ? cluster_name : host) << " / " << name;
    // On constitue le moyen de récupération des informations (avec les structures de LibcurlStruct)

    int attempt = 1;
    while (attempt <= read_attempts) {
        CURLcode res;
        struct curl_slist *list = NULL;
        DataStruct chunk;
        chunk.nbPassage = 0;
        chunk.data = (char *)malloc(1);
        chunk.size = 0;

        CURL *curl = CurlPool::get_curl_env();

        std::string fullUrl = url + "/" + bucket_name + "/" + name;

        time_t current;

        time(&current);
        struct tm *ptm = gmtime(&current);

        static char gmt_time[40];
        sprintf(
            gmt_time, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
            wday_name[ptm->tm_wday], ptm->tm_mday, mon_name[ptm->tm_mon], 1900 + ptm->tm_year,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

        std::string content_type = "application/octet-stream";
        std::string resource = "/" + bucket_name + "/" + name;
        std::string stringToSign = "GET\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
        std::string signature = getAuthorizationHeader(stringToSign);

        // Constitution du header

        char hd_host[256];
        sprintf(hd_host, "Host: %s", host.c_str());
        list = curl_slist_append(list, hd_host);

        char d[100];
        sprintf(d, "Date: %s", gmt_time);
        list = curl_slist_append(list, d);

        char ct[50];
        sprintf(ct, "Content-Type: %s", content_type.c_str());
        list = curl_slist_append(list, ct);

        std::string ex = "Expect:";
        list = curl_slist_append(list, ex.c_str());

        char auth[512];
        sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());

        list = curl_slist_append(list, auth);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if (ssl_no_verify) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        curl_slist_free_all(list);
        // delete[] gmt_time;

        if (CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            attempt++;
            sleep(waiting_time);
            continue;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code < 200 || http_code > 299) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
            attempt++;
            sleep(waiting_time);
            continue;
        }

        size = chunk.size;
        uint8_t *data = new uint8_t[chunk.size];
        memcpy(data, chunk.data, chunk.size);
        return data;
    }

    BOOST_LOG_TRIVIAL(error) <<  "Unable to full read S3 object" << bucket_name << "@" << ((cluster_name != "") ? cluster_name : host) << " / " << name << " after " << read_attempts << " tries" ;

    return NULL;
}

bool S3Context::write(uint8_t *data, int offset, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "S3 write : " << size << " bytes (from the " << offset << " one) in the writing buffer " << name;

    std::map<std::string, std::vector<char> *>::iterator it1 = write_buffers.find(name);
    if (it1 == write_buffers.end()) {
        // pas de buffer pour ce nom d'objet
        BOOST_LOG_TRIVIAL(error) << "No writing buffer for the name " << name;
        return false;
    }
    BOOST_LOG_TRIVIAL(debug) << "old length: " << it1->second->size();

    // Calcul de la taille finale et redimensionnement éventuel du vector
    if (it1->second->size() < size + offset) {
        it1->second->resize(size + offset);
    }

    memcpy(&((*(it1->second))[0]) + offset, data, size);
    BOOST_LOG_TRIVIAL(debug) << "new length: " << it1->second->size();

    return true;
}

bool S3Context::write_full(uint8_t *data, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "S3 write : " << size << " bytes (one shot) in the writing buffer " << name;

    std::map<std::string, std::vector<char> *>::iterator it1 = write_buffers.find(name);
    if (it1 == write_buffers.end()) {
        // pas de buffer pour ce nom d'objet
        BOOST_LOG_TRIVIAL(error) << "No S3 writing buffer for the name " << name;
        return false;
    }

    it1->second->clear();

    it1->second->resize(size);
    memcpy(&((*(it1->second))[0]), data, size);

    return true;
}

ContextType::eContextType S3Context::get_type() {
    return ContextType::S3CONTEXT;
}

std::string S3Context::get_type_string() {
    return "S3Context";
}

std::string S3Context::get_tray() {
    return bucket_name;
}

std::string S3Context::getCluster() {
    return cluster_name;
}

std::string S3Context::get_path(std::string racine, int x, int y, int pathDepth) {
    return racine + "_" + std::to_string(x) + "_" + std::to_string(y);
}

std::string S3Context::get_path(std::string name) {
    return bucket_name + "/" + name;
}

bool S3Context::open_to_write(std::string name) {
    std::map<std::string, std::vector<char> *>::iterator it1 = write_buffers.find(name);
    if (it1 != write_buffers.end()) {
        BOOST_LOG_TRIVIAL(error) << "A S3 writing buffer already exists for the name " << name;
        return false;
    } else {
        write_buffers.insert(std::pair<std::string, std::vector<char> *>(name, new std::vector<char>()));
    }

    return true;
}

bool S3Context::close_to_write(std::string name) {
    std::map<std::string, std::vector<char> *>::iterator it1 = write_buffers.find(name);
    if (it1 == write_buffers.end()) {
        BOOST_LOG_TRIVIAL(error) << "The S3 writing buffer with name " << name << "does not exist, cannot flush it";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "Write buffered " << it1->second->size() << " bytes in the S3 object " << name;

    int attempt = 1;
    while (attempt <= write_attempts) {

        CURLcode res;
        struct curl_slist *list = NULL;
        CURL *curl = CurlPool::get_curl_env();

        std::string fullUrl = url + "/" + bucket_name + "/" + name;

        time_t current;

        time(&current);
        struct tm *ptm = gmtime(&current);

        static char gmt_time[40];
        sprintf(
            gmt_time, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
            wday_name[ptm->tm_wday], ptm->tm_mday, mon_name[ptm->tm_mon], 1900 + ptm->tm_year,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

        std::string content_type = "application/octet-stream";
        std::string resource = "/" + bucket_name + "/" + name;
        std::string stringToSign = "PUT\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
        std::string signature = getAuthorizationHeader(stringToSign);

        // Constitution du header

        char hd_host[256];
        sprintf(hd_host, "Host: %s", host.c_str());
        list = curl_slist_append(list, hd_host);

        char d[100];
        sprintf(d, "Date: %s", gmt_time);
        list = curl_slist_append(list, d);

        char ct[50];
        sprintf(ct, "Content-Type: %s", content_type.c_str());
        list = curl_slist_append(list, ct);

        char cl[50];
        sprintf(cl, "Content-Length: %d", (int)it1->second->size());
        list = curl_slist_append(list, cl);

        std::string ex = "Expect:";
        list = curl_slist_append(list, ex.c_str());

        char auth[512];
        sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());
        list = curl_slist_append(list, auth);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if (ssl_no_verify) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &((*(it1->second))[0]));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, it1->second->size());

        res = curl_easy_perform(curl);
        curl_slist_free_all(list);

        if (CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            attempt++;
            sleep(waiting_time);
            continue;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code < 200 || http_code > 299) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
            attempt++;
            sleep(waiting_time);
            continue;
        }

        BOOST_LOG_TRIVIAL(debug) << "Erase the flushed buffer";
        delete it1->second;
        write_buffers.erase(it1);
        return true;
    }

    BOOST_LOG_TRIVIAL(error) <<  "Unable to flush " << it1->second->size() << " bytes in the S3 object " << bucket_name << "@" << ((cluster_name != "") ? cluster_name : host) << " / " << name << " after " << write_attempts << " tries" ;

    return true;
}

bool S3Context::exists(std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Exists (S3) ? " << get_path(name);

    CURLcode res;
    struct curl_slist *list = NULL;

    CURL *curl = CurlPool::get_curl_env();

    std::string fullUrl = url + "/" + bucket_name + "/" + name;

    time_t current;

    time(&current);
    struct tm *ptm = gmtime(&current);

    static char gmt_time[40];
    sprintf(
        gmt_time, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
        wday_name[ptm->tm_wday], ptm->tm_mday, mon_name[ptm->tm_mon], 1900 + ptm->tm_year,
        ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    std::string content_type = "application/octet-stream";
    std::string resource = "/" + bucket_name + "/" + name;
    std::string stringToSign = "HEAD\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
    std::string signature = getAuthorizationHeader(stringToSign);

    // Constitution du header

    char hd_host[256];
    sprintf(hd_host, "Host: %s", host.c_str());
    list = curl_slist_append(list, hd_host);

    char d[100];
    sprintf(d, "Date: %s", gmt_time);
    list = curl_slist_append(list, d);

    char ct[50];
    sprintf(ct, "Content-Type: %s", content_type.c_str());
    list = curl_slist_append(list, ct);

    char auth[512];
    sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());

    list = curl_slist_append(list, auth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    if (ssl_no_verify) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    res = curl_easy_perform(curl);

    curl_slist_free_all(list);

    if (CURLE_OK != res) {
        BOOST_LOG_TRIVIAL(error) << "Cannot test object existence from S3 : " << bucket_name + "/" + name;
        BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 200 && http_code <= 299) {
        return true;
    } else {
        return false;
    }
}