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
 * \file Attribute.h
 * \~french
 * \brief Définition de la classe Attribute gérant les attributs de couches de tuiles vecteur
 * \~english
 * \brief Define the Attribute class handling vector tiles' layers' attributes
 */

class Attribute;

#pragma once

#include <boost/algorithm/string/replace.hpp>
#include <vector>
#include <string>

#include "rok4/thirdparty/json11.hpp"

/**
 * \author Institut national de l'information géographique et forestière
 */
class Attribute
{

    public:
        Attribute(json11::Json doc) {

            missing_field = "";
            values = std::vector<std::string>();
            min_provided = false;
            max_provided = false;

            if (! doc["name"].is_string()) {
                missing_field = "name";
                return;
            }
            name = doc["name"].string_value();

            if (! doc["type"].is_string()) {
                missing_field = "type";
                return;
            }
            type = doc["type"].string_value();

            if (! doc["count"].is_number()) {
                missing_field = "count";
                return;
            }
            count = doc["count"].number_value();

            if (doc["min"].is_number()) {
                min = doc["min"].number_value();
                min_provided = true;
            }
            if (doc["max"].is_number()) {
                max = doc["max"].number_value();
                max_provided = true;
            }
            if (doc["values"].is_array()) {
                std::string tmp;
                for (json11::Json v : doc["values"].array_items()) {
                    tmp = v.string_value();
                    // on double les backslash, en évitant de traiter les backslash déjà doublés
                    boost::replace_all(tmp, "\\\\", "\\");
                    boost::replace_all(tmp, "\\", "\\\\");
                    // On échappe les doubles quotes
                    boost::replace_all(tmp, "\"", "\\\"");
                    values.push_back(tmp);
                }

            }

        };
        ~Attribute(){};

        std::string get_missing_field() {return missing_field;}
        std::string get_name() {return name;}
        std::string get_type() {return type;}
        std::vector<std::string> get_values() {return values;}
        int get_count() {return count;}
        int get_min() {return min;}
        int get_max() {return max;}

        json11::Json to_json() const {
            json11::Json::object res = json11::Json::object {
                { "attribute", name },
                { "count", count },
                { "type", type }
            };

            if (min_provided) {
                res["min"] = min;
            }
            if (max_provided) {
                res["max"] = max;
            }

            if (values.size() != 0) {
                res["values"] = values;
            }
            
            return res;
        }


    private:
        /**
         * \~french \brief Éventuel attribut manquant lors de la construction
         * \~english \brief Constructor missing field
         */
        std::string missing_field;

        /**
         * \~french \brief Nom de l'attribut
         * \~english \brief Attribute's name
         */
        std::string name;
        /**
         * \~french \brief Type de l'attribut
         * \~english \brief Attribute's type
         */
        std::string type;
        /**
         * \~french \brief Valeurs prises par l'attribut
         * \~english \brief Attribute's distinct values
         */
        std::vector<std::string> values;
        /**
         * \~french \brief Nombre de valeurs distinctes de l'attribut
         * \~english \brief Attribute's distinct values count
         */
        int count;
        /**
         * \~french \brief Valeur minimale prise par l'attribut si numérique
         * \~english \brief Min value of attribute if number
         */
        double min;
        bool min_provided;
        /**
         * \~french \brief Valeur maximale prise par l'attribut si numérique
         * \~english \brief Max value of attribute if number
         */
        double max;
        bool max_provided;
};



