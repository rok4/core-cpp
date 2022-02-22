# Librairies CORE Perl

- [Variables CMake](#variables-cmake)
- [Dépendances](#dépendances)
- [Utilisation en submodule GIT](#utilisation-en-submodule-git)

## Variables CMake

* `FILEIMAGE_ENABLED` : active la compilation des classes de lecture d'image fichier
* `KDU_ENABLED` : active la compilation avec le driver Kakadu pour la lecture des fichiers JPEG2000
* `OBJECT_ENABLED` : active la compilation des classes de gestion des stockages objet

## Dépendances

* Paquets debian
  * zlib1g-dev
  * libcurl4-openssl-dev
  * libproj-dev
  * libssl-dev
  * libturbojpeg0-dev
  * libjpeg-dev
  * libc6-dev
  * libjson11-1-dev
  * libboost-log-dev
  * libboost-filesystem-dev
  * libboost-system-dev
  * libsqlite3-dev
  * Si `FILEIMAGE_ENABLED` à 1
    * Si `KDU_ENABLED` à 0
      * libopenjp2-7-dev
    * libpng-dev
    * libtiff5-dev
  * Si `OBJECT_ENABLED` à 1
    * librados-dev

## Utilisation en submodule GIT

* Si le dépôt de code est à côté : `git submodule add ../core-cpp.git lib/core`
* Sinon : `git submodule add https://github.com/rok4/core-cpp.git lib/core`