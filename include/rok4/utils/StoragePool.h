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

#pragma once

#include "rok4/utils/StyleBook.h"
#include "rok4/utils/IndexCache.h"

#if CEPH_ENABLED
    #include "storage/ceph/CephPoolContext.h"
#endif

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un pool de contextes de stockage
 * \details Cette classe est prévue pour être utilisée sans instance
 */
class StoragePool {

private:

    /**
     * \~french
     * \brief Constructeur
     * \~english
     * \brief Constructeur
     */
    StoragePool();

    /**
     * \~french \brief Annuaire de contextes
     * \details La clé est une paire composée du type de stockage et du contenant du contexte
     * \~english \brief Book of contexts
     * \details Key is a pair composed of type of storage and the context's bucket
     */
    static std::map<std::pair<ContextType::eContextType,std::string>,Context*> pool;


public:


    /**
     * \~french \brief Retourne une chaîne de caracère décrivant l'annuaire
     * \~english \brief Return a string describing the pool
     */
    static std::string to_string();

    /**
     * \~french
     * \brief Récupère un contexte de stockage
     * \details Si un contexte existe déjà pour ce nom de contenant, on ne crée pas de nouveau contexte et on retourne celui déjà existant. Le nouveau contexte n'est pas connecté.
     * \param[in] type type de stockage pour lequel on veut créer un contexte
     * \param[in] tray Nom du contenant pour lequel on veut créer un contexte
     * \param[in] reference_context Contexte de stockage de référence

     * \brief Get a context storage
     * \details If a context already exists for this tray's name, we don't create a new one and the existing is returned. New context is not connected.
     * \param[in] type Storage Type for which context is created
     * \param[in] tray Tray's name for which context is created
     * \param[in] reference_context Reference storage context
     
     */
    static Context * get_context(ContextType::eContextType type,std::string tray, Context* reference_context = 0) ;


    /**
     * \~french \brief Retourne le nombre de contextes de stockage dans l'annuaire par type
     * \param[out] file_count Nombre de contexte de stockage fichier
     * \param[out] s3_count Nombre de contexte de stockage S3
     * \param[out] ceph_count Nombre de contexte de stockage Ceph
     * \param[out] swift_count Nombre de contexte de stockage Swift
     * \~english \brief Return the number of storage contexts in the book per type
     * \param[out] file_count File storage context count
     * \param[out] s3_count S3 storage context count
     * \param[out] ceph_count Ceph storage context count
     * \param[out] swift_count Swift storage context count
     */
    static void get_storages_count (int& file_count, int& s3_count, int& ceph_count, int& swift_count); 

    /**
     * \~french \brief Obtient l'annuaire de contextes
     * \details La clé est une paire composée du type de stockage et du contenant du contexte
     * \~english \brief Get book of contexts
     * \details Key is a pair composed of type of storage and the context's bucket
     */
    static std::map<std::pair<ContextType::eContextType,std::string>,Context*> get_pool();

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~StoragePool();

    /**
     * \~french \brief Nettoie tous les contextes de stockage dans l'annuaire et le vide
     * \~english \brief Clean all storage context objects in the book and empty it
     */
    static void clean_storages ();
};