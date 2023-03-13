Les grandes fonctionnalités offertes par cette librairie sont :

* Le chargement de pyramide via leur descripteur : cela implique le chargement du Tile Matrix Set utilisé
* La lecture de données, que ce soit des dalles de la pyramide ou des images brutes
* L'écriture de données, de dalles ROK4 (pour le stockage interne) ou d'images standard (pour la diffusion ou comme format de travail)
* La calcul raster :
    * reprojection / réechantillonnage : plusieurs noyaux d'interpolation sont disponibles
    * application de styles :
        * simples (le calcul de la valeur de destination d'un pixel ne dépend que de sa valeur d'origine)
        * complexes (le calcul de la valeur de destination d'un pixel dépend de sa valeur d'origine et de celle de ces voisins)
    * conversion des canaux
    * fusion de pixels
    * compression des données

Pour la partie de manipulation des données, 3 grandes classes mères sont implémentées :

* image : la lecture se fait ligne par ligne, calculée à la volée. Chaque sous classe aura sa façon de calculer la ligne à partir de sa source : lire dans un fichier (FileImage), reprojetée une image (ReprojectedImage), agrégé plusieurs images compatibles (même résolution, même phase : ExtendedCompoundImage)
* datasource : l'intégralité des données est chargée la première fois qu'on les demande
* datastream : la lecture des données se fait par morceau (interface orientée serveur de diffusion), à partir d'un buffer complet des données à lire (souvent issu d'un objet datasource).