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