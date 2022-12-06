# Librairies ROK4 core C++

## Summary

Le projet ROK4 a été totalement refondu, dans son organisation et sa mise à disposition. Les composants sont désormais disponibles dans des releases sur GitHub au format debian.

Cette release contient les librairies C++, utilisées par les outils de [generation](https://github.com/rok4/generation) et [le serveur de diffusion](https://github.com/rok4/server).

## Changelog

### [Added]

* Compilation pour alpine fonctionnelle

### [Changed]

* Les chargements des styles et des TMS passent par un système d'annuaire global. Ce dernier s'occupe de lire le fichier (stockage fichier ou objet) dans le cas d'une demande d'un nouveau, ou retourne celui déjà chargé

### [Fixed]

* Possibilité d'appliquer à la génération (dans mergeNtiff) un style définissant un calcul d'aspect ou de pente mais pas de palette (on reste alors avec un canal potentiellement flottant)
* Passage du nombre de jours sur 2 chiffres dans les appels S3

<!-- 
### [Added]

### [Changed]

### [Deprecated]

### [Removed]

### [Fixed]

### [Security] 
-->