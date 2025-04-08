/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file Style.h
 * \~french
 * \brief Définition de la classe Style modélisant les styles
 * \~english
 * \brief Define the Style Class handling style definition
 */

class Style;

#pragma once

#include <string>
#include <vector>
#include "rok4/utils/LegendURL.h"
#include "rok4/utils/Keyword.h"
#include "rok4/style/Palette.h"
#include "rok4/style/Pente.h"
#include "rok4/style/Estompage.h"
#include "rok4/style/Aspect.h"
#include "rok4/enums/Interpolation.h"
#include "rok4/utils/Configuration.h"
#include "rok4/enums/Format.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Style représente la façon de modifier des images.
 * Il est possible de définir une table de correspondance valeur/couleur ou un estompage.
 * Un style peut contenir uniquement des métadonnées ou définir plusieurs traitements.
 *
 * Exemple de fichier de style complet :
 * \brief Gestion des styles (affichages et métadonnées)
 * \~english
 * A Style represent the way to modify raster data.
 * Two types of data treatment are available, Lookup table to define a value/colour equivalence and relief shading
 *
 * Style file sample :
 * \brief Style handler (display and metadata)
 * \details \~ \code{.json}
 * {
 *     "identifier": "estompage",
 *     "title": "Estompage",
 *     "abstract": "Estompage Azimuth 315°",
 *     "keywords": ["MNT"],
 *     "legend": {
 *         "format": "image/png",
 *         "url": "http://ign.fr",
 *         "height": 100,
 *         "width": 100,
 *         "min_scale_denominator": 0,
 *         "max_scale_denominator": 30
 *     },
 *     "estompage": {
 *         "zenith": 45,
 *         "azimuth": 315,
 *         "z_factor": 1,
 *         "interpolation": "linear",
 *         "slope_nodata": 255,
 *         "slope_max": 254
 *     },
 *     "palette": {
 *         "no_alpha": false,
 *         "rgb_continuous": true,
 *         "alpha_continuous": true,
 *         "colours": [
 *             { "value": 0, "red": 0, "green": 0, "blue": 0, "alpha": 64 },
 *             { "value": 255, "red": 255, "green": 255, "blue": 255, "alpha": 64 }
 *         ]
 *     }
 * }
 * \endcode
 */
class Style : public Configuration {
private :
    /**
     * \~french \brief Identifiant interne du style
     * \details C'est le nom du fichier de style, sans extension. Il est utilisé dans les descripteurs de couches.
     * \~english \brief Internal style identifier
     * \details It's the style file name without extension. It used in layers' descriptors
     */
    std::string id;
    /**
     * \~french \brief Identifiant WMS/WMTS du style
     * \~english \brief WMS/WMTS style identifier
     */
    std::string identifier;
    /**
     * \~french \brief Liste des titres
     * \~english \brief List of titles
     */
    std::vector<std::string> titles;
    /**
     * \~french \brief Liste des résumés
     * \~english \brief List of abstracts
     */
    std::vector<std::string> abstracts;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keywords;
    /**
     * \~french \brief Liste des légendes
     * \~english \brief List of legends
     */
    std::vector<LegendURL> legends;
    /**
     * \~french \brief Table de correspondance (valeur -> couleur)
     * \~english \brief Lookup table (value -> colour)
     */
    Palette* palette;
    /**
     * \~french \brief Définit si un calcul de pente doit être appliqué
     * \~english \brief Define wether the server must compute a slope
     */
    Pente* pente;
    /**
     * \~french \brief Définit si un calcul d'exposition doit être appliqué
     * \~english \brief Define wether the server must compute an aspect
     */
    Aspect* aspect;
    /**
     * \~french \brief Définit si un estompage doit être appliqué
     * \~english \brief Define wether the server must compute a relief shadow
     */
    Estompage* estompage;

    /**
     * \~french \brief Valeur de nodata attendue dans les données en entrée
     * \~english \brief Nodata value expected in input data
     */
    int* input_nodata_value;

    /**
     * \~french \brief Valeur de nodata après style
     * \~english \brief Style nodata value
     */
    int* output_nodata_value;

    bool parse(json11::Json& doc);

public:

    /**
    * \~french
    * Crée un Style à partir d'un fichier JSON
    * \brief Constructeur
    * \param[in] path Chemin vers le descripteur de style
    * \~english
    * Create a Style from a JSON file
    * \brief Constructor
    * \param[in] path Path to JSON file
    */
    Style ( std::string path );

    /**
     * \~french
     * \brief Retourne l'identifiant du style
     * \return id
     * \~english
     * \brief Return the style's identifier
     * \return id
     */
    inline std::string get_id() {
        return id;
    }

    /**
     * \~french
     * \brief Retourne l'identifiant public du style
     * \return identifier
     * \~english
     * \brief Return the public style's identifier
     * \return identifier
     */
    inline std::string get_identifier() {
        return identifier;
    }
    /**
     * \~french \brief L'application du style est-elle possible ?
     * \~english \brief Style is allowed ?
     */
    bool handle (int spp) {
        if (estompage_defined() || pente_defined() || aspect_defined()) {
            return (spp == 1);
        } else {
            return true;
        }
    }

    /**
     * \~french \brief Combien de canaux en sortie du style
     * \~english \brief How many channels after style
     */
    int get_channels (int orig_channels) {
        if (palette && ! palette->is_empty()) {
            if (palette->is_no_alpha()) {
                return 3;
            } else {
                return 4;
            }
        } else {
            if (estompage_defined() || pente_defined() || aspect_defined()) {
                return 1;
            } else {
                // identité
                return orig_channels;
            }
        }
    }

    /**
     * \~french \brief Quel format de canal en sortie en sortie du style
     * \~english \brief Which sample format after style
     */
    SampleFormat::eSampleFormat get_sample_format (SampleFormat::eSampleFormat sf) {
        if (palette && ! palette->is_empty()) {
            return SampleFormat::UINT8;
        } else {
            return sf;
        }
    }

    /**
     * \~french \brief Valeur de nodata après style
     * \~english \brief Style nodata value
     */
    int* get_output_nodata_value (int* default_nodata) {
        if (is_identity()) {
            return default_nodata;
        } else {
            return output_nodata_value;
        }
    }

    /**
     * \~french \brief Valeur de nodata attendue dans les données en entrée
     * \~english \brief Nodata value expected in input data
     */
    int* get_input_nodata_value (int* default_nodata) {
        if (is_identity()) {
            return default_nodata;
        } else {
            return input_nodata_value;
        }
    }

    /**
     * \~french \brief Est ce que le style est une identité
     * \~english \brief Is style identity
     */
    bool is_identity () {
        if (palette && palette->get_colours_map() && ! palette->is_empty()) {
            return false;
        }

        if (estompage_defined() || pente_defined() || aspect_defined()) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * \~french
     * \brief Retourne la liste des titres
     * \return titres
     * \~english
     * \brief Return the list of titles
     * \return titles
     */
    inline std::vector<std::string> get_titles() {
        return titles;
    }

    /**
     * \~french
     * \brief Retourne la liste des résumés
     * \return résumés
     * \~english
     * \brief Return the list of abstracts
     * \return abstracts
     */
    inline std::vector<std::string> get_abstracts() {
        return abstracts;
    }

    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    inline std::vector<Keyword>* get_keywords() {
        return &keywords;
    }

    /**
     * \~french
     * \brief Retourne la liste des légendes
     * \return légendes
     * \~english
     * \brief Return the list of legends
     * \return legends
     */
    inline std::vector<LegendURL>* get_legends() {
        return &legends;
    }

    /**
     * \~french
     * \brief Retourne la table de correspondance
     * \return table de correspondance
     * \~english
     * \brief Return the lookup table
     * \return lookup table
     */
    inline Palette* get_palette() {
        return palette;
    }

    /**
     * \~french
     * \brief Détermine si le style décrit un estompage
     * \return true si oui
     * \~english
     * \brief Determine if the style describe a relief shadows
     * \return true if it does
     */
    inline bool estompage_defined() {
        return (estompage != 0);
    }
    /**
     * \~french
     * \brief Retourn l'estompage
     * \~english
     * \brief Return relief shadows
     */
    inline Estompage* get_estompage() {
        return estompage;
    }
	
    /**
     * \~french
     * \brief Return vrai si le style est une pente
     * \return bool
     * \~english
     * \brief Return true if the style is a slope
     * \return bool
     */
    inline bool pente_defined() {
        return (pente != 0);
    }
    /**
     * \~french
     * \brief Retourn la pente
     * \~english
     * \brief Return slope
     */
    inline Pente* get_pente() {
        return pente;
    }

   /**
    * \~french
    * \brief Return vrai si le style est une exposition
    * \return bool
    * \~english
    * \brief Return true if the style is an aspect
    * \return bool
    */
    inline bool aspect_defined() {
        return (aspect != 0);
    }
    /**
     * \~french
     * \brief Retourn l'aspect
     * \~english
     * \brief Return aspect
     */
    inline Aspect* get_aspect() {
        return aspect;
    }
	

    /**
     * \~french \brief Ajoute un noeud WMS correpondant à l'attribution
     * \param[in] parent Noeud auquel ajouter celui de l'attribution
     * \~english \brief Add a WMS node corresponding to attribution
     * \param[in] parent Node to whom add the attribution node
     */
    void add_node_wms(ptree& parent);

    /**
     * \~french \brief Ajoute un noeud WMTS correpondant à l'attribution
     * \param[in] parent Noeud auquel ajouter celui de l'attribution
     * \param[in] default_style Style par défaut
     * \~english \brief Add a WMTS node corresponding to attribution
     * \param[in] parent Node to whom add the attribution node
     * \param[in] default_style Default style
     */
    void add_node_wmts(ptree& parent, bool default_style = false);
	
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Style();

};



