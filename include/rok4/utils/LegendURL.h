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
 * \file LegendURL.h
 * \~french
 * \brief Définition de la classe LegendURL gérant les éléments de légendes des documents de capacités
 * \~english
 * \brief Define the LegendURL Class handling capabilities legends elements
 */

#ifndef LEGENDURL_H
#define LEGENDURL_H

#include <string>
#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;

#include "rok4/utils/ResourceLocator.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance LegendURL représente un élément LegendURL dans les différents documents de capacités.
 * \brief Gestion des éléments de légendes des documents de capacités
 * \~english
 * A LegendURL represent a LegendURL element in the differents capabilities documents.
 * \brief Legends handler for the capabilities documents
 */
class LegendURL : public ResourceLocator {
private:
    /**
     * \~french \brief Largeur de l'image
     * \~english \brief Image width
     */
    int width;
    /**
     * \~french \brief Hauteur de l'image
     * \~english \brief Image height
     */
    int height;
    /**
     * \~french \brief Échelle minimum à laquelle s'applique la légende
     * \~english \brief Minimum scale at which the legend is applicable
     */
    double min_scale_denominator;
    /**
     * \~french \brief Échelle maximum à laquelle s'applique la légende
     * \~english \brief Maximum scale at which the legend is applicable
     */
    double max_scale_denominator;
    
public:
    /**
     * \~french
     * \brief Crée un LegendURL à partir d'un élément JSON
     * \param[in] doc Élément JSON
     * \~english
     * \brief Create a LegendURL from JSON element
     * \param[in] doc JSON element
     */
    LegendURL ( json11::Json doc ) {
        missing_field = "";

        if (! doc["format"].is_string()) {
            missing_field = "format";
            return;
        }
        format = doc["format"].string_value();

        if (! doc["url"].is_string()) {
            missing_field = "url";
            return;
        }
        href = doc["url"].string_value();
        
        if (! doc["height"].is_number()) {
            missing_field = "height";
            return;
        }
        height = doc["height"].number_value();
        
        if (! doc["width"].is_number()) {
            missing_field = "width";
            return;
        }
        width = doc["width"].number_value();
        
        if (! doc["min_scale_denominator"].is_number()) {
            missing_field = "min_scale_denominator";
            return;
        }
        min_scale_denominator = doc["min_scale_denominator"].number_value();
        
        if (! doc["max_scale_denominator"].is_number()) {
            missing_field = "max_scale_denominator";
            return;
        }
        max_scale_denominator = doc["max_scale_denominator"].number_value();
    };
    /**
     * \~french
     * Crée un LegendURL à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] origLUrl LegendURL à copier
     * \~english
     * Create a LegendURL from another
     * \brief Copy Constructor
     * \param[in] origLUrl LegendURL to copy
     */
    LegendURL ( const LegendURL &origLUrl ) {
        href = origLUrl.href;
        format = origLUrl.format;
        height = origLUrl.height;
        width = origLUrl.width;
        max_scale_denominator = origLUrl.max_scale_denominator;
        min_scale_denominator = origLUrl.min_scale_denominator;
    };
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    LegendURL& operator= ( LegendURL const & other ) {
        if ( this != &other ) {
            ResourceLocator::operator= ( other );
            this->height = other.height;
            this->width = other.width;
            this->max_scale_denominator = other.max_scale_denominator;
            this->min_scale_denominator = other.min_scale_denominator;
        }
        return *this;
    };
    /**
     * \~french
     * \brief Test d'egalite de 2 LegendURLs
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 LegendURLs are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const LegendURL& other ) const {
        return ( this->width == other.width && this->height == other.height
                && this->min_scale_denominator == other.min_scale_denominator
                && this->max_scale_denominator == other.max_scale_denominator
                && this->get_format().compare ( other.get_format() ) == 0
                && this->get_href().compare ( other.get_href() ) == 0 );
    };
    /**
     * \~french
     * \brief Test d'inégalite de 2 LegendURLs
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 LegendURLs are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const LegendURL& other ) const {
        return ! ( *this == other );
    };

    /**
     * \~french
     * \brief Retourne la largeur de l'image
     * \return largeur
     * \~english
     * \brief Return the image width
     * \return width
     */
    inline int get_width() {
        return width;
    }

    /**
     * \~french
     * \brief Retourne la hauteur de l'image
     * \return hauteur
     * \~english
     * \brief Return the image height
     * \return height
     */
    inline int get_height() {
        return height;
    }

    /**
     * \~french
     * \brief Retourne l'échelle minimum
     * \return échelle minimum
     * \~english
     * \brief Return the minimum scale
     * \return minimum scale
     */
    inline double get_min_scale_denominator() {
        return min_scale_denominator;
    }

    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    inline double get_max_scale_denominator() {
        return max_scale_denominator;
    }

    /**
     * \~french \brief Ajoute un noeud WMTS correpondant à la légende
     * \param[in] parent Noeud auquel ajouter celui de la légende
     * \~english \brief Add a WMTS node corresponding to legend
     * \param[in] parent Node to whom add the legend node
     */
    void add_node_wmts(ptree& parent) {
        ptree& node = parent.add("LegendURL", "");
        node.add("<xmlattr>.format", format);
        node.add("<xmlattr>.xlink:href", href);

        if ( width != 0 ) {
            node.add("<xmlattr>.width", width);
        }
        if ( height != 0 ) {
            node.add("<xmlattr>.height", height);
        }
        if ( min_scale_denominator != 0 ) {
            node.add("<xmlattr>.minScaleDenominator", min_scale_denominator);
        }
        if ( max_scale_denominator != 0 ) {
            node.add("<xmlattr>.maxScaleDenominator", max_scale_denominator);
        }
    }

    /**
     * \~french \brief Ajoute un noeud WMS correpondant à la légende
     * \param[in] parent Noeud auquel ajouter celui de la légende
     * \~english \brief Add a WMS node corresponding to legend
     * \param[in] parent Node to whom add the legend node
     */
    void add_node_wms(ptree& parent) {
        ptree& node = parent.add("LegendURL", "");
        node.add("<xmlattr>.width", width);
        node.add("<xmlattr>.height", height);
        node.add("Format", format);
        node.add("OnlineResource.<xmlattr>.xlink:href", href);
        node.add("OnlineResource.<xmlattr>.xlink:type", "simple");
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~LegendURL() {};
};

#endif // LEGENDURL_H
