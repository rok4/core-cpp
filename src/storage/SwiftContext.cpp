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
 * \file SwiftContext.cpp
 ** \~french
 * \brief Implémentation de la classe SwiftContext
 * \details
 * \li SwiftContext : connexion à un container Swift
 ** \~english
 * \brief Implement classe SwiftContext
 * \details
 * \li SwiftContext : Swift container connection
 */

#include "storage/SwiftContext.h"
#include "utils/LibcurlStruct.h"
#include <curl/curl.h>
#include <sys/stat.h>
#include "utils/Cache.h"
#include <time.h>

SwiftContext::SwiftContext (std::string cont) : Context(), ssl_no_verify(false), keystone_auth(false), container_name(cont), use_token_from_file(true) {

    char* auth = getenv (ROK4_SWIFT_AUTHURL);
    if (auth == NULL) {
        auth_url.assign("http://localhost:8080/auth/v1.0");
    } else {
        auth_url.assign(auth);
    }

    char* user = getenv (ROK4_SWIFT_USER);
    if (user == NULL) {
        user_name.assign("tester");
    } else {
        user_name.assign(user);
    }

    char* passwd = getenv (ROK4_SWIFT_PASSWD);
    if (passwd == NULL) {
        user_passwd.assign("password");
    } else {
        user_passwd.assign(passwd);
    }

    char* publicu = getenv (ROK4_SWIFT_PUBLICURL);
    if (publicu == NULL) {
        public_url.assign("http://localhost:8080/api/v1");
    } else {
        public_url.assign(publicu);
    }

    if(getenv (ROK4_KEYSTONE_DOMAINID) != NULL){
        keystone_auth=true;
    }

    ssl_no_verify = get_ssl_no_verify();
}

bool SwiftContext::connection() {

    if (! connected) {

        // On va regarder si on a le token dans un fichier, pour éviter une authentification
        char* tf = getenv (ROK4_SWIFT_TOKEN_FILE);
        if (tf != NULL && use_token_from_file) {
            token_file.assign(tf);
            BOOST_LOG_TRIVIAL(debug) << "ROK4_SWIFT_TOKEN_FILE detected: " << token_file;

            std::fstream token_stream;
            token_stream.open(token_file, std::fstream::in);
            if (! token_stream) {
                token_stream.close();
                BOOST_LOG_TRIVIAL(debug) << "File " << token_file << " does not exist";
            }
            else if ( token_stream.is_open() ) {
                getline(token_stream, token);
                token_stream.close();
                BOOST_LOG_TRIVIAL(debug) << "File " << token_file << " exists: token loaded " << token;
                connected = true;
                return true;
            } else {
                token_stream.close();
                BOOST_LOG_TRIVIAL(warning) << "File " << token_file << " could not be opened";
            }
        }
        
        use_token_from_file = false;

        if (keystone_auth) {
            BOOST_LOG_TRIVIAL(debug) << "Keystone authentication";

            char* domain = getenv (ROK4_KEYSTONE_DOMAINID);
            if (domain == NULL) {
                BOOST_LOG_TRIVIAL(error) << "We need a domain id (ROK4_KEYSTONE_DOMAINID) for a keystone authentication";
                return false;
            } else {
                domain_id.assign(domain);
            }

            char* project = getenv (ROK4_KEYSTONE_PROJECTID);
            if (project == NULL) {
                BOOST_LOG_TRIVIAL(error) << "We need a project id (ROK4_KEYSTONE_PROJECTID) for a keystone authentication";
                return false;
            } else {
                project_id.assign(project);
            }

            if (token == "") {

                CURLcode res;
                struct curl_slist *list = NULL;
                CURL* curl = CurlPool::get_curl_env();

                curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
                if(ssl_no_verify){
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                }

                // On constitue le header

                const char* ct = "Content-Type: application/json";
                list = curl_slist_append(list, ct);

                // On constitue le body

                std::string body = "{ \"auth\": {\"scope\": { \"project\": {\"id\": \""+project_id+"\"}}, ";
                body += " \"identity\": { \"methods\": [\"password\"], \"password\": { \"user\": { \"domain\": { \"id\": \""+domain_id+"\"},";
                body += "\"name\": \""+user_name+"\", \"password\": \""+user_passwd+"\" } } } } }";

                HeaderStruct authHdr;
                DataStruct chunk;
                chunk.nbPassage = 0;
                chunk.data = (char*) malloc(1);
                chunk.size = 0;

                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &authHdr);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

                res = curl_easy_perform(curl);
                if( CURLE_OK != res) {
                    BOOST_LOG_TRIVIAL(error) << "Cannot authenticate to Keystone";
                    BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
                    curl_slist_free_all(list);
                    return false;
                }

                long http_code = 0;
                curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code < 200 || http_code > 299) {
                    BOOST_LOG_TRIVIAL(error) << "Cannot authenticate to Keystone";
                    BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
                    curl_slist_free_all(list);
                    return false;
                }

                // On récupère le token dans le header de la réponse
                token = std::string(authHdr.token);

                curl_slist_free_all(list);
            }
        } else {

            BOOST_LOG_TRIVIAL(debug) << "Swift authentication";

            char* account = getenv (ROK4_SWIFT_ACCOUNT);
            if (account == NULL) {
                BOOST_LOG_TRIVIAL(error) << "We need an account (ROK4_SWIFT_ACCOUNT) for a Swift authentication";
                return false;
            } else {
                user_account.assign(account);
            }

            CURLcode res;
            struct curl_slist *list = NULL;

            CURL* curl = CurlPool::get_curl_env();

            curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
            if(ssl_no_verify){
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            }

            // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

            char xUser[256];
            strcpy(xUser, "X-Storage-User: ");
            strcat(xUser, user_account.c_str());
            strcat(xUser, ":");
            strcat(xUser, user_name.c_str());

            char xPass[256];
            strcpy(xPass, "X-Storage-Pass: ");
            strcat(xPass, user_passwd.c_str());

            char xAuthUser[256];
            strcpy(xAuthUser, "X-Auth-User: ");
            strcat(xAuthUser, user_account.c_str());
            strcat(xAuthUser, ":");
            strcat(xAuthUser, user_name.c_str());

            char xAuthKey[256];
            strcpy(xAuthKey, "X-Auth-Key: ");
            strcat(xAuthKey, user_passwd.c_str());


            list = curl_slist_append(list, xUser);
            list = curl_slist_append(list, xPass);        
            list = curl_slist_append(list, xAuthUser);
            list = curl_slist_append(list, xAuthKey);

            HeaderStruct authHdr;

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &authHdr);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

            res = curl_easy_perform(curl);
            if( CURLE_OK != res) {
                BOOST_LOG_TRIVIAL(error) << "Cannot authenticate to Swift";
                BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
                curl_slist_free_all(list);
                return false;
            }

            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code < 200 || http_code > 299) {
                BOOST_LOG_TRIVIAL(error) << "Cannot authenticate to Swift";
                BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
                curl_slist_free_all(list);
                return false;
            }

            // On récupère le token dans le header de la réponse
            token = std::string(authHdr.token);

            curl_slist_free_all(list);
        }

        connected = true;
    }

    return true;
}

int SwiftContext::read(uint8_t* data, int offset, int size, std::string name) {

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Impossible de lire via un contexte non connecté";
        return -1;
    }

    BOOST_LOG_TRIVIAL(debug) << "Swift read : " << size << " bytes (from the " << offset << " one) in the object " << container_name << " / " << name;

    int attempt = 1;
    bool reconnection = false;
    while (attempt <= read_attempts) {
        
        CURLcode res;
        struct curl_slist *list = NULL;
        DataStruct chunk;
        chunk.nbPassage = 0;
        chunk.data = (char*) malloc(1);
        chunk.size = 0;

        int lastBytes = offset + size - 1;

        CURL* curl = CurlPool::get_curl_env();

        // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

        std::string fullUrl;
        fullUrl = public_url + "/" + container_name + "/" + name;

        char range[50];
        sprintf(range, "Range: bytes=%d-%d", offset, lastBytes);

        list = curl_slist_append(list, token.c_str());
        list = curl_slist_append(list, range);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if(ssl_no_verify){
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

        BOOST_LOG_TRIVIAL(debug) << "SWIFT READ START (" << size << ") " << pthread_self();
        res = curl_easy_perform(curl);
        BOOST_LOG_TRIVIAL(debug) << "SWIFT READ END (" << size << ") " << pthread_self();
        
        curl_slist_free_all(list);

        if( CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) << "Cannot read data from Swift : " << size << " bytes (from the " << offset << " one) in the object " << name;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            return -1;
        }

        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Nous avons un refus d'accès, cela peut venir d'une authentification expirée
        // Nous faisons une nouvelle demande de token et réessayons une fois (hors compte des tentatives de lecture)
        if ( ! reconnection && (http_code == 403 || http_code == 401 || http_code == 400) ) {
            BOOST_LOG_TRIVIAL(debug) << "Authentication may have expired. Reconnecting...";
            connected = false;
            reconnection = true;
            token = "";
            use_token_from_file = false;
            if (! connection()) {
                BOOST_LOG_TRIVIAL(error) << "Reconnection attempt failed.";
                return -1;
            }
            BOOST_LOG_TRIVIAL(debug) << "Successfully reconnected.";
            continue;
        }

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

    BOOST_LOG_TRIVIAL(error) <<  "Unable to read " << size << " bytes (from the " << offset << " one) from the Swift object " << container_name << " / " << name << " after " << read_attempts << " tries" ;
    return -1;
}


uint8_t* SwiftContext::read_full(int& size, std::string name) {

    size = -1;
    
    BOOST_LOG_TRIVIAL(debug) << "Swift read full : " << container_name << " / " << name;

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Try to read using the unconnected swift context " << container_name;
        return NULL;
    }

    int attempt = 1;
    bool reconnection = false;
    while (attempt <= read_attempts) {
        
        CURLcode res;
        struct curl_slist *list = NULL;
        DataStruct chunk;
        chunk.nbPassage = 0;
        chunk.data = (char*) malloc(1);
        chunk.size = 0;

        CURL* curl = CurlPool::get_curl_env();

        // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

        std::string fullUrl;
        fullUrl = public_url + "/" + container_name + "/" + name;

        list = curl_slist_append(list, token.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if(ssl_no_verify){
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

        res = curl_easy_perform(curl);
        
        curl_slist_free_all(list);

        if( CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) << "Cannot read full object from Swift : " << name;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            return NULL;
        }

        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Nous avons un refus d'accès, cela peut venir d'une authentification expirée
        // Nous faisons une nouvelle demande de token et réessayons une fois (hors compte des tentatives de lecture)
        if ( ! reconnection && (http_code == 403 || http_code == 401 || http_code == 400) ) {
            BOOST_LOG_TRIVIAL(debug) << "Authentication may have expired. Reconnecting...";
            connected = false;
            reconnection = true;
            token = "";
            use_token_from_file = false;
            if (! connection()) {
                BOOST_LOG_TRIVIAL(error) << "Reconnection attempt failed.";
                return NULL;
            }
            BOOST_LOG_TRIVIAL(debug) << "Successfully reconnected.";
            continue;
        }

        if (http_code < 200 || http_code > 299) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
            attempt++;
            sleep(waiting_time);
            continue;
        }

        uint8_t* data = new uint8_t[chunk.size];
        size = chunk.size;
        memcpy(data, chunk.data, chunk.size);
        return data;
    }

    BOOST_LOG_TRIVIAL(error) <<  "Unable to full read Swift object " << container_name << " / " << name << " after " << read_attempts << " tries" ;

    return NULL;
}

bool SwiftContext::write(uint8_t* data, int offset, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Swift write : " << size << " bytes (from the " << offset << " one) in the writing buffer " << name;

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
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

bool SwiftContext::write_full(uint8_t* data, int size, std::string name) {
    BOOST_LOG_TRIVIAL(debug) << "Swift write : " << size << " bytes (one shot) in the writing buffer " << name;

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
        // pas de buffer pour ce nom d'objet
        BOOST_LOG_TRIVIAL(error) << "No Swift writing buffer for the name " << name;
        return false;
    }

    it1->second->clear();

    it1->second->resize(size);
    memcpy(&((*(it1->second))[0]), data, size);

    return true;
}

ContextType::eContextType SwiftContext::get_type() {
    return ContextType::SWIFTCONTEXT;
}

std::string SwiftContext::get_type_string() {
    return "SWIFTCONTEXT";
}

std::string SwiftContext::get_tray() {
    return container_name;
}

bool SwiftContext::open_to_write(std::string name) {

    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 != write_buffers.end() ) {
        BOOST_LOG_TRIVIAL(error) << "A Swift writing buffer already exists for the name " << name;
        return false;

    } else {
        write_buffers.insert ( std::pair<std::string,std::vector<char>*>(name, new std::vector<char>()) );
    }

    return true;
}


bool SwiftContext::close_to_write(std::string name) {


    std::map<std::string, std::vector<char>*>::iterator it1 = write_buffers.find ( name );
    if ( it1 == write_buffers.end() ) {
        BOOST_LOG_TRIVIAL(error) << "The Swift writing buffer with name " << name << "does not exist, cannot flush it";
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "Write buffered " << it1->second->size() << " bytes in the Swift object " << name;


    int attempt = 1;
    bool reconnection = false;
    while (attempt <= write_attempts) {
        CURLcode res;
        struct curl_slist *list = NULL;
        CURL* curl = CurlPool::get_curl_env();

        // On constitue le header

        std::string fullUrl;
        fullUrl = public_url + "/" + container_name + "/" + name;

        list = curl_slist_append(list, token.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        if(ssl_no_verify){
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &((*(it1->second))[0]));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, it1->second->size());

        res = curl_easy_perform(curl);
        curl_slist_free_all(list);

        if( CURLE_OK != res) {
            BOOST_LOG_TRIVIAL(error) <<  "Try " << attempt << " failed" ;
            BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
            attempt++;
            sleep(waiting_time);
            continue;
        }

        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Nous avons un refus d'accès, cela peut venir d'une authentification expirée
        // Nous faisons une nouvelle demande de token et réessayons une fois (hors compte des tentatives de lecture)
        if ( ! reconnection && (http_code == 403 || http_code == 401 || http_code == 400) ) {
            BOOST_LOG_TRIVIAL(debug) << "Authentication may have expired. Reconnecting...";
            connected = false;
            reconnection = true;
            token = "";
            use_token_from_file = false;
            if (! connection()) {
                BOOST_LOG_TRIVIAL(error) << "Reconnection attempt failed.";
                return false;
            }
            BOOST_LOG_TRIVIAL(debug) << "Successfully reconnected.";
            continue;
        }


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

    BOOST_LOG_TRIVIAL(error) <<  "Unable to flush " << it1->second->size() << " bytes in the Swift object " << name << " after " << write_attempts << " tries" ;

    return false;
}

std::string SwiftContext::get_path(std::string racine,int x,int y,int pathDepth){
    return racine + "_" + std::to_string(x) + "_" + std::to_string(y);
}

std::string SwiftContext::get_path(std::string name) {  
    return container_name + "/" + name;
}


bool SwiftContext::exists(std::string name) {

    BOOST_LOG_TRIVIAL(debug) << "Exists (SWIFT) ? " << get_path(name);

    if (! connected) {
        BOOST_LOG_TRIVIAL(error) << "Try to test object existence using the unconnected swift context " << container_name;
        return false;
    }
        
    CURLcode res;
    struct curl_slist *list = NULL;
    CURL* curl = CurlPool::get_curl_env();

    std::string fullUrl;
    fullUrl = public_url + "/" + container_name + "/" + name;

    list = curl_slist_append(list, token.c_str());

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
    if(ssl_no_verify){
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    res = curl_easy_perform(curl);
    
    curl_slist_free_all(list);

    if( CURLE_OK != res) {
        BOOST_LOG_TRIVIAL(error) << "Cannot test object existence from SWIFT : " << container_name + "/" + name;
        BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 200 && http_code <= 299) {
        return true;
    } else {
        return false;
    }

}