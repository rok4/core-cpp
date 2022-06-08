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
 * \file StoreDataSource.h
 ** \~french
 * \brief Définition des classes StoreDataSource et StoreDataSourceFactory
 * \details
 * \li StoreDataSource : Permet de lire de la donnée quelque soit le type de stockage
 * \li StoreDataSourceFactory : usine de création d'objet StoreDataSource
 ** \~english
 * \brief Define classes StoreDataSource and StoreDataSourceFactory
 * \details
 * \li StoreDataSource : To read data, whatever the storage type
 * \li StoreDataSourceFactory : factory to create StoreDataSource object
 */

#ifndef STOREDATASOURCE_H
#define STOREDATASOURCE_H

#include "datasource/DataSource.h"
#include "storage/Context.h"
#include <stdlib.h>
#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Lecture d'une source de données
 * \details Cette classe abstraite permet de lire de la donnée quelque soit le contexte de stockage :
 * \li Fichier -> FileContext
 * \li Ceph -> CepĥPoolContext
 * \li Swift -> SwiftContext
 */
class StoreDataSource : public DataSource {

protected:
    /**
     * \~french \brief Nom de la source de donnée
     * \~english \brief Data source name
     */
    std::string name;

    /**
     * \~french \brief Contexte de stockage d'origine
     * \~english \brief Original storage context
     */
    Context* context;

    /**
     * \~french \brief Offset de la donnée voulue
     * \~english \brief Data offset
     */
    uint32_t offset;
    /**
     * \~french \brief Taille de la donnée voulue
     * \~english \brief Data size
     */
    uint32_t wanted_size;

    /****** Quand on veut lire une tuile dans une dalle ******/
    /**
     * \~french \brief Numéro de la tuile dans la dalle
     * \~english \brief Tile indice in the slab
     */
    int tile_indice;
    /**
     * \~french \brief Nombre de tuile dans la dalle
     * \~english \brief Slab tiles number
     */
    int tiles_number;
    /*********************************************************/

    /**
     * \~french \brief Donnée (tuile) voulue
     * \details Si elle est demandée plusieurs fois, on ne va la lire dans la source qu'une seule fois
     * \~english \brief Wanted data (a tile)
     * \details If asked serveral times, data source is read only once
     */
    uint8_t* data;
    /**
     * \~french \brief A-t-on déjà essayé de lire la donnée
     * \~english \brief Have we already tried to read data
     */
    bool alreadyTried;
    /**
     * \~french \brief Taille utile dans #data
     * \~english \brief Real size in #data
     */
    size_t size;
    /**
     * \~french \brief Encodage de la donnée
     * \~english \brief Data encoding
     */
    std::string encoding;
    /**
     * \~french \brief Mime-type de la donnée
     * \~english \brief Data mime-type
     */
    std::string type;

public:

    /** \~french
     * \brief Crée un objet StoreDataSource à lecture partielle générique
     * \details On renseigne directement les offset et taille de la donnée à lire
     * \param[in] n Nom de la source de donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] o Offset de la donnée à lire
     * \param[in] s Taille de la donnée à lire
     * \param[in] type Mime-type de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, for partially reading.
     * \param[in] n Data source name
     * \param[in] c Data storage context
     * \param[in] o Data's offset
     * \param[in] s Data's size
     * \param[in] type Data mime-type
     * \param[in] encoding Data encoding
     */
    StoreDataSource ( std::string n, Context* c, const uint32_t o, const uint32_t s, std::string type, std::string encoding = "");
    
    /** \~french
     * \brief Crée un objet StoreDataSource pour lire une tuile selon l'index de la dalle
     * \details On renseigne le numéro de la tuile et le nombre totale de tuiles, permettant de récupérer l'offset et la taille de la tuile à lire
     * \param[in] tile_ind Numéro de la tuile dans l'index
     * \param[in] tiles_nb Nombre d'élément dans l'index
     * \param[in] n Nom de la source de donnée
     * \param[in] c Contexte de stockage de la donnée
     * \param[in] type Mime-type de la donnée
     * \param[in] encoding Encodage de la source de donnée
     ** \~english
     * \brief Create a StoreDataSource object, to read a tile thanks to slab index
     * \param[in] tile_ind Tile's indice
     * \param[in] tiles_nb Element number in the index
     * \param[in] n Data source name
     * \param[in] c Data storage context
     * \param[in] type Data mime-type
     * \param[in] encoding Data encoding
     */
    StoreDataSource (const int tile_ind, const int tiles_nb, std::string n, Context* c, std::string type, std::string encoding = "");

    /** \~french
     * \brief Récupère la donnée depuis la source
     * \details Si la donnée a déjà été lue (#data est déjà instancié), on la retourne directement.
     * 
     * Dans le cas d'une lecture complète, on essaie de lire #maxsize dans la source, et on mémorise la taille effectivement lue
     * 
     * Dans le cas d'une lecture partielle, on lit la position de la donnée (#posoff), la taille de la donnée (#possize), et on lit la tuile.
     * \param[out] tile_size Taille utile dans le buffer pointé en sortie
     * \return Un pointeur vers la donnée
     ** \~english
     * \brief Get the data from the source
     * \details If data is already read (#data is not null), it's returned without re-reading.
     * 
     * For full reading, we try to read #maxsize in data source, and we memorize the real read size.
     * 
     * For partially reading, We read tile's position (#posoff), tile's size (#possize), then we read the data.
     * \param[out] tile_size Real size of data in the returned pointed buffer
     * \return Data pointer
     */
    virtual const uint8_t* getData ( size_t &tile_size );


    /**
     * \~french \brief Supprime la donnée mémorisée (#data)
     * \~english \brief Delete memorized data (#data)
     */
    bool releaseData() {
        if (data) {
            delete[] data;
        }
        data = 0;
        return true;
    }

    /**
     * \~french \brief Destructeur
     * \details Appelle #releaseData
     * \~english \brief Destructor
     * \details Call #releaseData
     */
    ~StoreDataSource(){
        releaseData();
    }

    /**
     * \~french \brief Retourne 200
     * \~english \brief Return 200
     */
    int getHttpStatus() {
        return 200;
    }

    /**
     * \~french \brief Retourne la taille des données
     * \~english \brief Return data size
     */
    unsigned int getLength() {
        return size;
    }

    /**
     * \~french \brief Retourne l'encodage #encoding
     * \~english \brief Return #encoding
     */
    std::string getEncoding() {
        return encoding;
    }

    /**
     * \~french \brief Retourne le mime-type #type
     * \~english \brief Return the mime-type #type
     */
    std::string getType() {
        return type;
    }

};

#endif // STOREDATASOURCE_H