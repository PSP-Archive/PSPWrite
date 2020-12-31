
  PSPWrite : un editeur de texte pour PSP

 Ecrit par Ludovic.Jacomme plus connu sous le pseudo Zx-81 (zx81.zx81@gmail.com)

1. INTRODUCTION
   ------------

  PSPWrite est un petit editeur de texte sans pretention (rien a voir avec
  Open office) mais qui permet d'ecrire n'importe quel fichier sur votre 
  PSP.

  Le code source est inclus, et cette archive est distribuée sous licence GNU,
  vous pouvez lire le fichier COPYING.txt pour plus d'informations.


2. INSTALLATION
   ------------

  Pour l'installer rien de plus simple, il vous suffit de "dezipper" 
  l'archive dans le repertoire game, game150 ou gameXXX en fonction
  de votre firmware (fw1.5, fw3.x, fw5.x etc ...)

  Cet homebrew a ete developpe sous linux pour les firmwares 5.x-M33
  et il doit normalement fonctionner aussi sur les PSP slim.

  Un grand merci a Danzel pour son clavier virtuel.

  Pour tous commentaires vous pouvez poser vos question sur
  mon blog : http://zx81.zx81.free.fr


3. LES TOUCHES
   ------------

3.1 - Clavier virtuel

  Dans la fenetre de l'editeur, il suffit d'appuyer sur
  Start pour ouvrir ou ferme le clavier virtuel.

  En utilisant le joystick / pad analogique vous pouvez
  deplacer la selection sur une des 9 cases du clavier.

  Ensuite en appuyant sur X, O, [] ou triangle vous 
  selectionnez la lettre correspondante. 

  En utiliser les gachettes L et R vous pourrez changer
  entre plusieurs claviers virtuels (pour les symboles
  ou les lettres accentuees etc ...)
  
  Quand vous etes dans le clavier virtuel le pad numerique
  est actif de la facon suivante :

  Gauche  deplace le curseur a gauche
  Droite  deplace le curseur a droite
  Haut    debut de ligne
  Bas     retour chariot
  Select  desactive le clavier virtuel
  Start   desactive le clavier virtuel

3.2 - Les touches standards

  Quand le clavier virtuel n'est plus affiche les 
  touches sont utilisables de la facon suivante :

  Haut        Deplace le curseur en haut
  Bas         Deplace le curseur en bas
  Gauche      Deplace le curseur a gauche
  Droite      Deplace le curseur a droite
  Triangle    Backspace
  Carre       Efface
  Cercle      Espace
  Croix       Retour chariot
  Select      Menu
  Start       Clavier virtuel
  
  L+Haut      Premiere ligne
  L+Bas       Derniere ligne
  L+Gauche    Debut de ligne
  L+Droite    Fin de ligne

  L+Select    Menu d'aide

  L+Triangle  Premiere ligne
  L+Croix     Derniere ligne
  L+Carre     Debut de ligne
  L+Cercle    Fin de ligne
  
  R+Haut      Page en Haut
  R+Bas       Page en Bas
  R+Gauche    Mot a Gauche
  R+Droite    Mot a Droite

  R+Select    Mode selection

  R+Triangle  Copier
  R+Croix     Couper
  R+Carre     Rewrap le paragraphe
  R+Cercle    Coller


3.3 - Le clavier sans fil

  Vous pouvez utiliser un clavier sans (voir sur mon blog
  pour les modeles qui fonctionnent).

  Vous pouvez editer le fichier pspirkeyb.ini ainsi que
  les fichiers presents dans le repertoire keymap pour 
  adapter PSPWrite a votre clavier.

  Le clavier fonctionne de la facon suivante :

  Clavier IR    PSP

  Curseur       Pad Numerique

  Tab           Tab   
  Ctrl-W        Touche Start

  Escape        Bascule en mode commande
  Ctrl-Q        Touche Select

  Ctrl-E        Touche Triangle
  Ctrl-X        Touche Croix
  Ctrl-S        Touche Carre
  Ctrl-F        Touche Cercle

  Ctrl-L        Remet a zero la ligne
  Ctrl-C        Copier
  Ctrl-V        Mode selection
  Ctrl-D        Couper
  Ctrl-P        Coller
  Ctrl-B        Mot a gauche
  Ctrl-N        Mot a droite

4. Options & configuration
   ------------

  Le menu de l'editeur vous permet de changer quelques options
  En particulier la couleur de texte et de fond de l'editeur
  le mode DOS ou unix pour la sauvegarde des fichiers texte 
  et la taille des tabulations (lorsqu'elles sont expanssees)
  

5. COMPILATION
   ------------

  Cet homebrew a ete developpe sous linux avec gcc et le PSPSDK.
  Pour le reconstruire a partir des sources il vous suffit 
  d'utilser le makefile fournit

  Amusez vous bien,
  
         Zx
