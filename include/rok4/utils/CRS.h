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
 * \file CRS.h
 * \~french
 * \brief Définition de la gestion des systèmes de référence
 * \~english
 * \brief Define the reference systems handler
 */

#ifndef CRS_H
#define CRS_H

#include <string>
#include "rok4/utils/BoundingBox.h"

/**
 * \~french \brief Code utilisé en cas de non correspondance avec les référentiel de Proj
 * \~english \brief Used code when no corresponding Proj code is found
 */
#define NO_PROJ_CODE "NO_PROJ_CODE"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un CRS permet de faire le lien entre un identifiant de CRS issue d'une requête WMS et l'identifiant utilisé dans la bibliothèque Proj.
 * Des fonctions de gestion des emprises sont disponibles (reprojection, recadrage sur l'emprise de définition)
 * \brief Gestion des systèmes de référence
 * \~english
 * A CRS allow to link the WMS and the Proj library identifiers.
 * Functions are availlable to work with bounding boxes (reprojection, cropping on the definition area)
 * \brief Reference systems handler
 */

class CRS {
private:

    /**
     * \~french \brief Code du CRS tel qu'il est ecrit dans la requete WMS
     * \~english \brief CRS identifier from the WMS request
     */
    std::string request_code;
    /**
     * \~french \brief Code du CRS dans la base proj
     * \~english \brief CRS identifier from Proj registry
     */
    std::string proj_code;
    /**
     * \~french \brief Emprise de définition du CRS en géographique
     * \~english \brief CRS's definition area
     */
    BoundingBox<double> definition_area;
    /**
     * \~french \brief Emprise de définition du CRS en natif
     * \~english \brief CRS's definition area
     */
    BoundingBox<double> native_definition_area;

    PJ* pj_proj;

    static CRS epsg4326;

public:
    /**
     * \~french
     * \brief Crée un CRS sans correspondance avec une entrée du registre PROJ
     * \~english
     * \brief Create a CRS without Proj correspondance
     */
    CRS();
    /**
     * \~french
     * \brief Crée un CRS à partir de son identifiant
     * \details La chaîne est comparée, sans prendre en compte la casse, avec les registres de Proj. Puis la zone de validité est récupérée dans le registre.
     * \param[in] crs_code identifiant du CRS
     * \~english
     * \brief Create a CRS from its identifier
     * \details The string is compared, case insensitively, to Proj registry. Then the corresponding definition area is fetched from Proj.
     * \param[in] crs_code CRS identifier
     */
    CRS ( std::string crs_code );
    /**
     * \~french
     * Crée un CRS à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] crs CRS à copier
     * \~english
     * Create a CRS from another
     * \brief Copy Constructor
     * \param[in] crs CRS to copy
     */
    CRS ( CRS* crs );
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    CRS& operator= ( CRS const& other );
    /**
     * \~french
     * \brief Test d'egalite de 2 CRS
     * \return true s'ils ont le meme code Proj, false sinon
     * \~english
     * \brief Test whether 2 CRS are equals
     * \return true if they share the same Proj identifier
     */
    bool operator== ( const CRS& crs ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 CRS
     * \return true s'ils ont un code Proj différent, false sinon
     * \~english
     * \brief Test whether 2 CRS are different
     * \return true if the the Proj identifier is different
     */
    bool operator!= ( const CRS& crs ) const;
    /**
     * \~french
     * \brief Test si le CRS est géographique
     * \return vrai si géographique
     * \~english
     * \brief Test whether the CRS is geographic
     * \return true if geographic
     */
    bool is_geographic();
    /**
     * \~french
     * \brief Test si les coordonnées sont dans l'ordre latitude longitude
     * \~english
     * \brief Test if coordinates are latitude/longitude order
     */
    bool is_lat_lon();
    /**
     * \~french
     * \brief Renvoit le zone de définition du CRS
     * \return BoundingBox représentant la zone en EPSG:4326
     * \~english
     * \brief Return the definition area of the CRS
     * \return BoundingBox representing the zone in EPSG:4326
     */
    BoundingBox<double> get_crs_definition_area() {
        return definition_area;
    };

    /**
     * \~french
     * \brief Renvoit le zone de définition native du CRS
     * \return BoundingBox représentant la zone en native
     * \~english
     * \brief Return the native definition area of the CRS
     * \return BoundingBox representing the zone in CRS
     */
    BoundingBox<double> get_native_crs_definition_area() {
        return native_definition_area;
    };

    /**
     * \~french
     * \brief Le nombre de mètre par unité du CRS
     * \return rapport entre le mètre et l'unité du CRS
     * \todo Supporter les CRS autres qu'en degré et en mètre
     * \~english
     * \brief Amount of meter in one CRS's unit
     * \return quotient between meter and CRS's unit
     * \todo Support all CRS types not only projected in meter and geographic in degree
     */
    long double get_meters_per_unit();

    /**
     * \~french
     * \brief Compare le code fournit lors de la création du CRS avec la chaîne
     * \param[in] crs chaîne à comparer
     * \return vrai si identique (insenble à la casse)
     * \~english
     * \brief Compare the CRS original code with the supplied string
     * \param[in] crs string for comparison
     * \return true if identic (case insensitive)
     */
    bool cmp_request_code ( std::string crs );
    /**
     * \~french
     * \brief Retourne l'authorité du CRS
     * \return l'identifiant de l'authorité
     * \~english
     * \brief Return the CRS authority
     * \return the authority identifier
     */
    std::string get_authority(); // Renvoie l'autorite du code passe dans la requete WMS (Ex: EPSG,epsg,IGNF,etc.)
    /**
     * \~french
     * \brief Retourne l'identifiant du CRS sans l'authorité
     * \return l'identifiant du système
     * \~english
     * \brief Return the CRS identifier without the authority
     * \return the system identifier
     */
    std::string get_identifier();// Renvoie l'identifiant du code passe dans la requete WMS (Ex: 4326,LAMB93,etc.)

    /**
     * \~french
     * \brief Test si le CRS possède un équivalent dans Proj
     * \return vrai si disponible dans Proj
     * \~english
     * \brief Test whether the CRS has a Proj equivalent
     * \return true if available in Proj
     */
    bool inline is_define() {
        return proj_code != NO_PROJ_CODE;
    }

    /**
     * \~french
     * \brief Retourne le code du CRS tel qu'il est ecrit dans la requete WMS
     * \return code du CRS
     * \~english
     * \brief Return the CRS identifier from the WMS request
     * \return CRS identifier
     */
    std::string inline get_request_code() {
        return request_code;
    }

    /**
     * \~french
     * \brief Retourne le code du CRS dans la base proj
     * \return code du CRS
     * \~english
     * \brief Return the CRS identifier from Proj registry
     * \return CRS identifier
     */
    std::string inline get_proj_code() {
        return proj_code;
    }

    /**
     * \~french
     * \brief Retourne l'instance PROJ
     * \~english
     * \brief Return PROJ instance
     */
    PJ* get_proj_instance() {
        return pj_proj;
    }
    
    /**
     * \~french
     * \brief Retourne la valeur d'un paramètre de la définition complète du CRS dans la base projCode
     * \param[in] param le nom du paramètre
     * \return la valeur du paramètre
     * \~english
     * \brief Return the value of a parameter in complete definition of the CRS from Proj registry
     * \param[in] param parameter name
     * \return parameter value
     */
    std::string get_proj_param( std::string paramName );

    /**
     * \~french
     * \brief Dis si un paramètre existe
     * \param[in] param le nom du paramètre
     * \return bool
     * \~english
     * \brief Return true if a parameter exists
     * \param[in] param parameter name
     * \return bool
     */
    bool test_proj_param( std::string paramName );
    

    /**
     * \~french
     * \brief Retourne le CRS EPSG:4326
     * \return CRS
     * \~english
     * \brief Return the CRS EPSG:4326
     * \return CRS
     */
    static CRS* get_epsg4326();

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~CRS();

};

#endif

