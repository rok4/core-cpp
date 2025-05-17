## 2.0.4

### [Fixed]

* `Merge` : correction du nombre de méthode

## 2.0.3

### [Added]

* `TiffHeader` : la valeur de nodata est ajoutée dans le header des Geotiff

### [Fixed]

* `Style` : gestion d'une valeur par défaut pour la demande de nodata, en entrée et en sortie

## 2.0.2

### [Fixed]

* `TiffDeflateEncoder` : l'encodage du résultat final n'est pas en deflate (seule la donnée dans le tiff l'est, et non le résultat complet)
* `S3Context` : on retourne bien false lorsqu'une erreur est rencontrée lors du flush (close_to_write)


## 2.0.1

### [Added]

* `Keyword` : ajout d'un exporteur JSON (API Tiles)
* `TileMatrixLimits` : ajout d'un exporteur JSON (API Tiles)

## 2.0.0

### [Added]

* Internalisation de la lib json
* Ajout d'exporteur XML (via la lib boost) pour les entités Style, BoundingBox, Keyword, LegendURL, TileMatrixLimit et Style
* Création d'un annuaire pour les CRS pour éviter les créations en double

### [Changed]

* Passage complet en snake case
* Le format de canal contient le nombre de bits d'encodage
* Renommage StyledImage -> PaletteImage
* Renommage lzwEncoder -> LzwCompressor
* Renommage lzwDecoder -> LzwUncompressor
* Renommage pkbEncoder -> PkbCompressor
* Renommage pkbDecoder -> PkbUncompressor

### [Deprecated]

Liste de fonctionnalités dépréciées.

### [Removed]

* Suppression de la notion 'inspire' dans la gestion des styles
* Suppression de la classe ConvertedChannelsImage
* Suppression de la gestion de la compilation avec la librairie Kakadu
* Suppression des factory pour les classes filles de Image
* Suppression de la fonctionnalité de crop dans Rok4Image

### [Fixed]

* Correction du nettoyage des annuaires de TMS et styles


## 1.4.0

### [Added]

* Stockage objet (S3, Swift et Ceph) 
    * Possibilité de définir un nombre de tentatives pour les lectures (1 par défaut) : variable d'environnement `ROK4_OBJECT_READ_ATTEMPTS` 
    * Possibilité de définir un nombre de tentatives pour les écritures (1 par défaut) : variable d'environnement `ROK4_OBJECT_WRITE_ATTEMPTS` 
    * Possibilité de définir un temps d'attente, en secondes, entre les tentatives (5 par défaut) : variable d'environnement `ROK4_OBJECT_ATTEMPTS_WAIT`

## 1.3.1

### [Fixed]

* `S3Context` : pour éviter des conflits dans une utilisation multithreadée, la sortie de la fonction HMAC (openssl) est dédiée.

## 1.3.0

### [Added]

* Gestion du multi cluster S3 :
    * nom du cluster = hôte du cluster avec le port (pas de protocole)
    * Pour préciser le cluster auquel on s'adresse, le nom du bucket aura la forme `<nom du bucker>@<nom du cluster>`
    * Les variables d'environnement `ROK4_S3_URL`, `ROK4_S3_KEY` et `ROK4_S3_SECRETKEY` peuvent contenir une liste de valeurs séparées par des virgules
    * **Dans** les descripteurs de pyramide et leur liste ou les objets symboliques, on peut ne pas préciser le nom du cluster : on sait alors qu'on travaille sur le même cluster que celui de l'objet d'origine

### [Fixed]

* Attribute : dans les valeurs des attributs, on échappe les éventuelles back quotes

## 1.2.4

### [Fixed]

* `Cache` : les modifications dans le cache quand il n'est pas par thread (index des dalles, TMS et styles) se font en exclusion mutuelle (mutex lock et unlock)

## 1.2.3

### [Fixed]

* `Level` : Ajout de pixels de marge lors de la reprojection des données d'un niveau de pyramide

## 1.2.2

### [Fixed]

* `LibopenjpegImage` : la lecture des images JPEG 2000 tuilées recharge l'image à la lecture de chaque tuile

## 1.2.1

### [Fixed]

* `LegendURL` : la fonction de copie d'une instance recopie bien le format et le href
* `LibtiffImage` : correction du calcul de nombre de tuile dans la largeur lors de la lecture d'une image dont la largeur est un multiple de la taille de la tuile 
* `BoundingBox` : lorsque l'on met en phase une bbox, les 4 bords doivent être traités indépendemment les uns des autres (avec un calcul de phase pour chacun)

### [Changed]

* La variable d'environnement `ROK4_TMS_NO_CACHE` permet de désactiver le cache de chargement des TMS
* La variable d'environnement `ROK4_STYLES_NO_CACHE` permet de désactiver le cache de chargement des styles

## 1.1.2

### [Changed]

* Le test d'existence d'un objet ou d'un fichier n'est plus une lecture de 1 octet mais une implémentation spécifique à chaque type
* Les TMS et les styles sont cherchés sur le stockage avec et sans extension JSON

### [Fixed]

* Table
    * Correction d'une typo dans l'écriture du metadata.json : filedsCount -> fieldsCount
* Style
    * La valeur de nodata en sortie d'un style est la première valeur de la palette (et non la couleur pour la valeur 0)

## 1.1.1

### [Fixed]

* LibopenjpegImage
    * Fixe sur la lecture des images à tuile unique suite au zonage

## 1.1.0

### [Added]

* LibtiffImage
    * Capacité à lire des images tuilées
    * Capacité à lire des images avec palette

### [Changed]

* LibopenjpegImage
    * Lecture des images à tuile unique par paquet de 256 lignes
    * Lecture des images tuilées par tuile


## 1.0.3

Les librairies sont gérées de manière indépendantes, conditionnées pour être installées en tant que dépendance dynamique. Le projet a son propre site, avec toutes les versions et leur documentation.

### [Added]

* Librairie, partie `utils` :
    * Styles et TMS sont chargés dans un annuaire, qui connait le dossier de stockage des fichiers / objets les définissant
    * Les pyramide sont chargées depuis leur descripteur, fichier ou objet
* Librairie, partie `storage` : gère un annuaire de contextes de stockages, fichier, Swift, S3 ou Ceph
* Librairie, partie `image` : permet la lecture et le calcul ligne par ligne
* Librairie, partie `datasource` : permet la lecture en une fois d'un buffer de donnée
* Librairie, partie `datasource` : permet la lecture par morceau d'un buffer de donnée
* Librairie, partie `processors` : gère plusieurs noyaux d'interpolation et la conversion de pixel

* Intégration continue :
    * Compilation du fichier librok4.so et conditionnement dans des paquets debian avec les headers, avec et sans la prise en charge du stockage ceph
    * Jeu des tests unitaires
    * Compilation de la documentation et publication sur la branche gh-pages

* Ajout du mode DEBUG à la compilation