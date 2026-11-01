# Aide de ZeroSky

La vue du ciel suit votre position et l'heure. Sur l'appareil, elle peut aussi suivre l'IMU. H ouvre cette page, F/X la font défiler et Esc la ferme.

## Pages et déplacement

- 4 fait défiler Ciel, Vue, Cam et Rég. Maintenez 4 pour la page précédente.
- Sur l'appareil, TAB bascule entre le pilotage par l'IMU et la caméra manuelle. Sur l'ordinateur, la vue est toujours manuelle.
- Hors d'Aligner, les flèches déplacent la vue manuelle de 5 degrés. En mode inclinaison seule, les flèches gauche/droite alignent le nord arbitraire du gyroscope sur le ciel réel. Haut/bas suivent l'IMU. F/X/Z/C servent aussi de haut/bas/gauche/droite hors saisie de texte.

## Page Ciel

- 5 ouvre Lieu. Saisissez une ville, une latitude et une longitude, ou choisissez une position GPS. Utilisez haut/bas et Enter pour sélectionner. Lieu s'ouvre automatiquement à la première utilisation.
- 6 dézoome, 7 zoome. Les deux touches sont désactivées quand la caméra est allumée. Le zoom de Ciel est enregistré quand vous quittez l'application.
- 8, marquée d'une loupe, ouvre Chercher. Tapez une partie du nom d'un objet, choisissez une ligne et appuyez sur Enter. Pendant le guidage, 8 affiche un signe stop : appuyez dessus pour arrêter le guidage et les bips. Un anneau marque un objet à l'écran. Une flèche au bord indique un objet hors de la vue. Des bips plus rapides et plus aigus signifient que vous êtes plus près.

## Page Vue

- 5 bascule entre le thème nuit rouge et le thème jour bleu.
- 6 fait défiler le filtrage diurne. Une demi-étoile (Dim) masque les étoiles faibles de jour, une étoile pleine (All) les montre.
- 7 fait défiler les figures des constellations : masquées, lignes seules, puis lignes avec les noms latins. L'icône passe d'un œil barré à des lignes reliées, puis à une liste d'étoiles.
- 8 affiche ou masque les 605 symboles et noms du ciel profond. Chercher peut toujours localiser un objet masqué. Les deux choix de Vue sont enregistrés d'un lancement à l'autre.

## Page Cam

- 5, marquée d'une caméra ou d'une caméra barrée, allume ou éteint le fond caméra en direct. La première image peut prendre quelques secondes. Les lignes, les étoiles et les étiquettes restent visibles sur la photo.
- 6, marquée d'un réticule ou d'une coche, entre dans Aligner ou le termine. De nuit, visez des étoiles connues. Poussez une étoile dessinée sur l'étoile photographiée correspondante : droite déplace le dessin vers la droite, gauche vers la gauche, et haut/bas de la même façon. Faites d'abord coïncider une étoile près du centre, puis utilisez 7 et 8 pour resserrer ou élargir le champ de l'objectif jusqu'à ce qu'une autre étoile près d'un bord coïncide.
- Dans Aligner, chaque flèche change le décalage de visée de 0.5 degré. 7 et 8 changent le champ de l'objectif de 1 degré. Appuyez de nouveau sur 6 sur **Terminé** pour enregistrer, seulement si vous avez changé quelque chose. Éteindre la caméra ou quitter avec Esc annule les changements d'Aligner non terminés.
- Tant qu'aucun alignement n'est enregistré, la ligne d'état affiche FOV estimé quand la caméra est allumée hors d'Aligner. Le champ initial de l'objectif, 62.2 degrés, est une estimation.
- Le thème nuit/jour, le filtrage diurne, l'état de la caméra et le zoom de Ciel sont conservés d'un lancement à l'autre. Le zoom de Ciel est enregistré à la sortie. **Terminé** enregistre l'alignement de l'objectif séparément.

## Page Rég

- 5 choisit la langue : Auto, EN, IT, FR, DE, ES, JA, ZH. L'icône de traduction signifie Auto, qui suit le système. Redémarrez ZeroSky pour appliquer la nouvelle langue.
- 6 fait défiler le profil matériel magnétique : un panneau d'interdiction pour none, une antenne pour cap, une prise USB pour USB GPS. Utilisez la configuration réellement branchée.
- 7 ouvre À propos, le QR code du manuel complet et Crédits.
- 8, marquée d'une boussole, ouvre en plein écran la calibration magnétique pour le profil matériel choisi. En extérieur, loin du métal, déplacez l'appareil en dessinant un 8 tout en le tournant sur chaque face. Huit points montrent la couverture des directions. La barre montre la progression vers 200 échantillons indépendants et le temps restant. La capture s'arrête quand chaque octant a au moins 15 échantillons, ou après 120 secondes. Une coche et deux tons montants indiquent que l'ajustement est prêt : Enter enregistre, Esc annule. Un ton grave et une raison indiquent que l'ajustement a échoué.

## Langue

- L'interface, cette aide et le manuel existent en anglais, italien, français, allemand, espagnol, japonais et chinois simplifié. Une police CJK du système complète le texte japonais et chinois.
- La page Rég enregistre le choix dans ~/.config/zeroradio/lang, un fichier commun à toute la suite ZeroRadio. Pour l'instant, seul ZeroSky le lit. ZERORADIO_LANG a priorité sur ce fichier. Avec Auto, LC_ALL, LC_MESSAGES et LANG décident.

## Ligne d'état

- Az et Alt donnent la direction de la vue. FOV est le champ de vision horizontal. Le nom du lieu suit.
- manuel : le pilotage par l'IMU est désactivé, les flèches commandent la caméra.
- axes non mesurés : enregistrez la correspondance des repères de l'appareil avec zerosky_probe.
- pas d'IMU : la détection du capteur a échoué.
- inclinaison seule : la hauteur et le roulis suivent l'IMU. L'azimut suit le gyroscope, mais le nord est arbitraire et peut dériver. Utilisez les flèches gauche/droite pour l'aligner. Une calibration valide faite en extérieur pour le profil matériel choisi active le pilotage vers le nord vrai.
- boussole perturbée : le champ magnétique mesuré ne correspond pas au champ attendu. L'IMU peut encore piloter la vue. Éloignez-vous du métal ou des appareils électroniques, ou appuyez sur TAB pour passer en commande manuelle.

## Calibration

- Mesurez d'abord les axes du capteur, magnétomètre compris. Enregistrez-les avec zerosky_probe --save-frames.
- Réglez une position, choisissez le profil matériel avec Rég : 6, puis appuyez sur la boussole de Rég : 8 en extérieur, loin du métal. Déplacez l'appareil en dessinant un 8 tout en le tournant sur chaque face. La fenêtre affiche les échantillons distincts et les octants couverts. Elle s'arrête quand la couverture est complète ou après 120 secondes.
- Un ajustement réussi affiche le RMS et le champ WMM. Enter enregistre, Esc annule sans changer l'ancien profil. Si l'ajustement échoue, suivez la raison affichée et recommencez. Le fichier docs/probe.md du dépôt des sources décrit la mesure des axes.
