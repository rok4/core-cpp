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

#include "rok4/utils/ResourceLocator.h"
#include <string>

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
    double minScaleDenominator;
    /**
     * \~french \brief Échelle maximum à laquelle s'applique la légende
     * \~english \brief Maximum scale at which the legend is applicable
     */
    double maxScaleDenominator;
    
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
        missingField = "";

        if (! doc["format"].is_string()) {
            missingField = "format";
            return;
        }
        format = doc["format"].string_value();

        if (! doc["url"].is_string()) {
            missingField = "url";
            return;
        }
        href = doc["url"].string_value();
        
        if (! doc["height"].is_number()) {
            missingField = "height";
            return;
        }
        height = doc["height"].number_value();
        
        if (! doc["width"].is_number()) {
            missingField = "width";
            return;
        }
        width = doc["width"].number_value();
        
        if (! doc["min_scale_denominator"].is_number()) {
            missingField = "min_scale_denominator";
            return;
        }
        minScaleDenominator = doc["min_scale_denominator"].number_value();
        
        if (! doc["max_scale_denominator"].is_number()) {
            missingField = "max_scale_denominator";
            return;
        }
        maxScaleDenominator = doc["max_scale_denominator"].number_value();
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
        height = origLUrl.height;
        width = origLUrl.width;
        maxScaleDenominator = origLUrl.maxScaleDenominator;
        minScaleDenominator = origLUrl.minScaleDenominator;
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
            this->maxScaleDenominator = other.maxScaleDenominator;
            this->minScaleDenominator = other.minScaleDenominator;
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
                && this->minScaleDenominator == other.minScaleDenominator
                && this->maxScaleDenominator == other.maxScaleDenominator
                && this->getFormat().compare ( other.getFormat() ) == 0
                && this->getHRef().compare ( other.getHRef() ) == 0 );
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
    inline int getWidth() {
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
    inline int getHeight() {
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
    inline double getMinScaleDenominator() {
        return minScaleDenominator;
    }

    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    inline double getMaxScaleDenominator() {
        return maxScaleDenominator;
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
