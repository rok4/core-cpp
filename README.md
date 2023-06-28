# Librairies ROK4 C++

![ROK4 Logo](https://rok4.github.io/assets/images/rok4.png)

Ces librairies facilitent la manipulation d'entités du projet ROK4 comme les Tile Matrix Sets ou les pyramides, mais aussi la manipulation des données : lecture et écriture des dalles, réechantillonnage et reprojection de données raster. 4 types de stockages sont gérés : fichier, S3, Swift et optionnellement Ceph.

- [Installer la librairie (Debian)](#installer-la-librairie-debian)
- [Utiliser la librairie](#utiliser-la-librairie)
    - [Variables d'environnement utilisées](#variables-denvironnement-utilisées)
    - [Hello ROK4 !](#hello-rok4-)
    - [Dans un projet CMake](#dans-un-projet-cmake)
- [Compiler la librairie (Debian)](#compiler-la-librairie-debian)
    - [Dépendances supplémentaires](#dépendances-supplémentaires)
    - [Variables CMake](#variables-cmake)
    - [Compilation, tests unitaires et documentation et installation](#compilation-tests-unitaires-et-documentation-et-installation)


## Installer la librairie (Debian)

Installations système requises (listées dans le paquet debian, installées avec la librairie lors du `apt install`) :

* `zlib1g-dev`
* `libcurl4-openssl-dev`
* `libproj-dev`
* `libssl-dev`
* `libturbojpeg0-dev`
* `libjpeg-dev`
* `libc6-dev`
* `libjson11-1-dev`
* `libboost-log-dev`
* `libboost-filesystem-dev`
* `libboost-system-dev`
* `libsqlite3-dev`
* `libpng-dev`
* `libtiff5-dev`
* `libopenjp2-7-dev`
* `librados-dev` (uniquement dans le cas d'une installation intégrant le stockage Ceph)

Depuis [GitHub](https://github.com/rok4/core-cpp/releases/) : 
```
curl -o librok4-dev.deb https://github.com/rok4/core-cpp/releases/download/x.y.z/librok4-base-x.y.z-ubuntu-20.04-amd64.deb
# or, with ceph driver
curl -o librok4-dev.deb https://github.com/rok4/core-cpp/releases/download/x.y.z/librok4-ceph-x.y.z-ubuntu-20.04-amd64.deb

apt install ./librok4-dev.deb
```

## Utiliser la librairie

### Variables d'environnement utilisées

Leur définition est contrôlée à l'usage.

* Pour le chargement de la configuration (non obligatoire, possibilité de surcharger via des appels)
    - `ROK4_TMS_DIRECTORY` : dossier (fichier ou objet) contenant les TMS. Le TMS `PM` sera chargé depuis le fichier/objet `<ROK4_TMS_DIRECTORY>/PM.json`
    - `ROK4_STYLES_DIRECTORY` : dossier (fichier ou objet) contenant les styles. Le style `normal` sera chargé depuis le fichier/objet `<ROK4_STYLES_DIRECTORY>/normal.json`
* Pour le stockage S3
    - `ROK4_S3_URL`
    - `ROK4_S3_KEY`
    - `ROK4_S3_SECRETKEY`
* Pour le stockage SWIFT
    - `ROK4_SWIFT_AUTHURL`
    - `ROK4_SWIFT_USER`
    - `ROK4_SWIFT_PASSWD`
    - `ROK4_SWIFT_PUBLICURL`
    - Si authentification via Swift
        - `ROK4_SWIFT_ACCOUNT`
    - Si connection via keystone (présence de `ROK4_KEYSTONE_DOMAINID`)
        - `ROK4_KEYSTONE_DOMAINID`
        - `ROK4_KEYSTONE_PROJECTID`
    - `ROK4_SWIFT_TOKEN_FILE` afin de sauvegarder le token d'accès, et ne pas le demander si ce fichier en contient un
* Pour configurer l'usage de libcurl (intéraction SWIFT et S3)
    - `ROK4_SSL_NO_VERIFY`
    - `HTTP_PROXY`
    - `HTTPS_PROXY`
    - `NO_PROXY`
* Pour le stockage CEPH
    - `ROK4_CEPH_CONFFILE`
    - `ROK4_CEPH_USERNAME`
    - `ROK4_CEPH_CLUSTERNAME`

### Hello ROK4 !

Aucune variable d'environnement requise dans cet exemple.

Le programme qui suit charge une pyramide SCAN1000 à partir de son descripteur, et calcule une image reprojetée en 4326.

```cpp
#include <boost/log/trivial.hpp>
#include <rok4/utils/Pyramid.h>
#include <rok4/image/file/FileImage.h>

int main( int argc, char *argv[] ) {

    BOOST_LOG_TRIVIAL(info) << "Hello ROK4 !";

    Pyramid* p = new Pyramid("/path/to/SCAN1000.json");
    int error = 0;
    CRS* crs_dst = new CRS("EPSG:4326");
    Image* img = p->getbbox(10, 10, BoundingBox<double>(5., 45., 6., 46.), 200, 200, crs_dst, false, Interpolation::KernelType::LANCZOS_3, 0, error);

    FileImageFactory FIF;
    FileImage* output = FIF.createImageToWrite(
        "hello.tif", img->getBbox(), img->getResX(), img->getResY(), img->getWidth(), img->getHeight(),
        p->getChannels(), p->getSampleFormat(), p->getBitsPerSample(), p->getPhotometric(), Compression::eCompression::DEFLATE
    );

    if (output == NULL) {
        return 1;
    }

    if (output->writeImage(img) < 0) {
        return 1;
    }

    // Clean

    delete p;
    delete img;
    delete output;
    delete crs_dst;

    TmsBook::send_to_trash();
    TmsBook::empty_trash();

    ProjPool::cleanProjPool();
    proj_cleanup();

    IndexCache::cleanCache();
    StoragePool::cleanStoragePool();

    return 0;
}
```

La phase de nettoyage est longue car les lectures et reprojection passent par des annuaires d'entité (TMS, stockage, contextes PROJ) pour limiter la création d'objets et l'empreinte mémoire (orienté performance pour un serveur). Il faut alors les vider à la fin pour éviter les fuites mémoires.

Commande de compilation
```bash
g++ \
  -DBOOST_LOG_DYN_LINK \
  -std=c++11 \
  -Wall \
  main.cpp \
  -lboost_log_setup \
  -lboost_log \
  -lrok4 \
  -lproj \
  -pthread \
  -o hellorok4
```

Appel :
```
~ $ ./hellorok4
~ $ tiffinfo hello.tif 
TIFF Directory at offset 0x1c626 (116262)
  Image Width: 200 Image Length: 200
  Bits/Sample: 8
  Sample Format: unsigned integer
  Compression Scheme: AdobeDeflate
  Photometric Interpretation: RGB color
  Samples/Pixel: 3
  Rows/Strip: 16
  Planar Configuration: single image plane
```

### Dans un projet CMake

Fichier CMake `FindRok4.cmake`
```
# If it's found it sets ROK4_FOUND to TRUE
# and following variables are set:
#    ROK4_INCLUDE_DIR
#    ROK4_LIBRARY

FIND_PATH(ROK4_INCLUDE_DIR enums/Format.h 
    /usr/local/include/rok4
    /usr/include/rok4
    c:/msys/local/include/rok4
    )

FIND_LIBRARY(ROK4_LIBRARY NAMES librok4.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64
    c:/msys/local/lib
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Rok4" DEFAULT_MSG ROK4_INCLUDE_DIR ROK4_LIBRARY )
```

## Compiler la librairie (Debian)

### Dépendances supplémentaires

* `build-essential`
* `cmake`
* Pour les tests unitaires
    * `libcppunit-dev`
* Pour la documentation
    * `doxygen`
    * `graphviz`

`apt install build-essential cmake libcppunit-dev doxygen graphviz` 

### Variables CMake

* `KDU_ENABLED` : active la compilation avec le driver Kakadu pour la lecture des fichiers JPEG2000. Valeur par défaut : `0`
* `KDU_THREADING` : renseigne le niveau de parallélisation dans le cas de l'utilisation de Kakadu. Valeur par défaut : `0`
* `CEPH_ENABLED` : active la compilation la classe de gestion du stockage Ceph. Valeur par défaut : `0`, `1` pour activer.
* `UNITTEST_ENABLED` : active la compilation des tests unitaires. Valeur par défaut : `1`, `0` pour désactiver.
* `DOC_ENABLED` : active la compilation de la documentation. Valeur par défaut : `1`, `0` pour désactiver.
* `BUILD_VERSION` : version de la librairie compilée. Valeur par défaut : `0.0.0`. Utile pour la compilation de la documentation.
* `DEBUG_BUILD` : active la compilation en mode debug. Valeur par défaut : `0`, `1` pour activer.

### Compilation, tests unitaires et documentation et installation

```bash
mkdir build && cd build
cmake -DBUILD_VERSION=0.0.0 -DCMAKE_INSTALL_PREFIX=/opt/rok4 -DCEPH_ENABLED=1 ..
make
make test
make doc
make install
```
 
