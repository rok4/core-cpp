# Librairies CORE CPP

- [Variables CMake](#variables-cmake)
- [Compilation et tests unitaires](#compilation-et-tests-unitaires)
- [Dépendances](#dépendances)
- [Utilisation en submodule GIT](#utilisation-en-submodule-git)
- [Variables d'environnement utilisées dans les librairies](#variables-denvironnement-utilisées-dans-les-librairies)

## Variables CMake

Dans le cas d'une compilation autonome :

* `FILEIMAGE_ENABLED` : active la compilation des classes de lecture d'image fichier. Valeur par défaut : `1`, `0` pour désactiver.
* `KDU_ENABLED` : active la compilation avec le driver Kakadu pour la lecture des fichiers JPEG2000
* `KDU_THREADING` : renseigne le niveau de parallélisation dans le cas de l'utilisation de Kakadu. Valeur par défaut : `0`
* `OBJECT_ENABLED` : active la compilation des classes de gestion des stockages objet. Valeur par défaut : `1`, `0` pour désactiver.
* `UNITTEST_ENABLED` : active la compilation des tests unitaires. Valeur par défaut : `1`, `0` pour désactiver.


## Compilation et tests unitaires

```bash
mkdir build && cd build
cmake ..
make
make test
```

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
  * Si `UNITTEST_ENABLED` à `1`
    * libcppunit-dev

## Utilisation en submodule GIT

* Si le dépôt de code est à côté : `git submodule add ../core-cpp.git lib/core`
* Sinon : `git submodule add https://github.com/rok4/core-cpp.git lib/core`
 
## Variables d'environnement utilisées dans les librairies

Leur définition est contrôlée à l'usage.

* Pour le stockage CEPH
    - `ROK4_CEPH_CONFFILE`
    - `ROK4_CEPH_USERNAME`
    - `ROK4_CEPH_CLUSTERNAME`
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
