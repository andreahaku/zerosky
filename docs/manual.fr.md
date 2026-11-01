# Manuel d'utilisation de ZeroSky

ZeroSky est un planétarium de poche pour le M5Stack CardputerZero. Il dessine le ciel pour l'heure actuelle de l'appareil et le lieu choisi, avec un thème nuit rouge et un thème jour bleu. L'IMU peut piloter la vue avant même la calibration magnétique, avec une direction du nord arbitraire. Après la calibration, la boussole fournit un cap par rapport au nord vrai. Vous pouvez aussi explorer avec les touches ou afficher l'image de la caméra en direct sous le ciel calculé.

## Pour commencer

1. Ouvrez ZeroSky depuis le lanceur du CardputerZero. À la première utilisation, la boîte de dialogue Lieu apparaît. Choisissez une ville, saisissez des coordonnées ou attendez une position GPS, puis appuyez sur Enter.
2. Vérifiez l'horloge de l'appareil. Les positions dans le ciel utilisent l'heure du système. Choisir une position GPS ne règle pas l'horloge.
3. Utilisez les flèches, ou F/X/Z/C, pour explorer en mode manuel. Appuyez sur 4 pour faire défiler les pages Ciel, Vue, Cam et Rég de la barre inférieure.
4. Pour que la vue suive l'appareil, mesurez et enregistrez les axes de ses capteurs. Sans calibration magnétique, `inclinaison seule` suit la hauteur et le roulis, et les flèches gauche/droite alignent le cap. Calibrez le magnétomètre en extérieur pour activer le pilotage vers le nord vrai. Le [guide de la sonde de terrain](probe.md) donne les commandes.

## Lire le ciel

Le centre de l'écran est la direction de visée actuelle. L'azimut se mesure dans le sens horaire depuis le nord vrai : 0° nord, 90° est, 180° sud et 270° ouest. La hauteur vaut 0° sur l'horizon et elle est positive au-dessus. La ligne d'état affiche `Az`, `Alt`, le `FOV` horizontal et le lieu choisi.

La taille des étoiles dépend de leur éclat. Des lignes relient les figures des constellations occidentales. Elles sont illustratives et ne correspondent pas aux limites officielles de l'UAI. Leurs noms latins apparaissent aux points d'étiquette de d3-celestial. Les objets du ciel profond ont des symboles de carte et des noms de catalogue. Le Soleil, la Lune et les planètes portent une étiquette. Le sol sombre et la ligne d'horizon montrent où le ciel rejoint le plan horizontal local. L'affichage utilise des directions calculées et un horizon simple. Les bâtiments et le relief proches ne sont pas modélisés.

Avec le filtrage diurne activé, la limite des étoiles change progressivement pendant que le Soleil descend à travers les crépuscules civil, nautique et astronomique. C'est une aide à l'affichage, pas une mesure de la luminosité locale du ciel. Les nuages, l'éclairage urbain et la Lune peuvent changer ce que vous voyez réellement. Désactivez le filtrage diurne sur la page Vue pour afficher le catalogue de nuit pendant la journée. Les lignes des constellations et les objets du Système solaire restent dessinés selon la vue actuelle.

## Touches

| Touche | Page Ciel | Page Vue | Page Cam | Page Rég |
| --- | --- | --- | --- | --- |
| 4 | Page suivante, maintenir pour la précédente | Page suivante, maintenir pour la précédente | Page suivante, maintenir pour la précédente | Page suivante, maintenir pour la précédente |
| 5 | Ouvrir Lieu | Basculer entre thème nuit et thème jour | Fond caméra allumé/éteint | Choisir la langue |
| 6 | Dézoomer (caméra éteinte) | Changer le filtrage diurne : demi-étoile (Dim) ou étoile pleine (All) | Aligner/Terminé | Choisir le matériel magnétique `none`, `cap` ou `usb_gps` |
| 7 | Zoomer (caméra éteinte) | Faire défiler les constellations : masquées, lignes, lignes avec noms latins | Resserrer le champ de l'objectif dans Aligner | Ouvrir À propos, le QR code du manuel et Crédits |
| 8 | Ouvrir Chercher (loupe) ; pendant le guidage, arrêter (signe stop) | Afficher/masquer les symboles et noms du ciel profond | Élargir le champ de l'objectif dans Aligner | Ouvrir la calibration de la boussole |

Hors d'Aligner, chaque appui sur une flèche tourne la vue manuelle de 5°. En `inclinaison seule`, les flèches gauche/droite règlent le décalage de cap. La hauteur et le roulis suivent l'IMU. Sur le CardputerZero, F/X/Z/C servent aussi de haut/bas/gauche/droite hors d'un champ de texte. Sur l'appareil, TAB bascule entre le pilotage par l'IMU et la commande manuelle. Le simulateur sur ordinateur est toujours manuel. H ouvre une page d'aide plus courte. ESC ferme une boîte de dialogue ou une page d'aide ouverte, ou quitte l'application quand aucune fenêtre n'est ouverte. Dans l'aide et dans À propos, F/X font défiler le texte et Z/C changent de page quand c'est possible.

La page Cam affiche une icône de caméra pour **5** quand l'image est éteinte et une caméra barrée quand elle est allumée. **6** affiche un réticule pour Aligner et une coche pour Terminé. Les numéros des touches restent les mêmes quand l'icône change.

Sur la page Vue, **7** affiche un œil barré quand les constellations sont masquées, des lignes reliées pour les figures seules et une liste d'étoiles pour les figures avec noms latins. **8** affiche des cercles quand les symboles et noms du ciel profond sont actifs, ou un cercle en pointillés quand ils sont désactivés. Les noms et les symboles peuvent être omis là où ils gêneraient une autre étiquette. Par défaut, les lignes des constellations avec leurs noms et le ciel profond sont actifs. Les deux choix sont conservés dans `~/.config/zerosky/settings`. La liste de Chercher et son guidage incluent toujours les objets du ciel profond quand leur affichage est désactivé.

Caméra éteinte, le champ de vision horizontal va de 15° à 120°. Un champ plus étroit agrandit le ciel et montre des étoiles du catalogue plus faibles quand le filtrage diurne les autorise. Les touches de zoom de Ciel sont désactivées quand la caméra est allumée. Le zoom de Ciel est enregistré quand vous quittez l'application, pas après chaque appui.

## Caméra et alignement de l'objectif

Sur la page **Cam**, appuyez sur **5** pour afficher l'image de l'IMX219 derrière les étoiles calculées, les lignes des constellations, l'horizon et le guidage. La première image peut prendre quelques secondes. Les lignes et les étiquettes ont un contour ou un fond sombre pour rester lisibles sur une scène claire. Si la caméra est absente ou si son image n'est plus mise à jour, le fond du ciel calculé reste utilisable. Appuyez de nouveau sur **5** pour libérer la caméra. Quand elle est allumée, le zoom de Ciel est désactivé et la vue utilise le champ de l'objectif. La valeur de départ, 62.2° à l'horizontale, est une estimation tirée de la Pi Camera v2, pas une mesure de cet exemplaire. Tant que vous n'avez pas enregistré d'alignement, la ligne d'état affiche `FOV estimé` quand la caméra est allumée et qu'Aligner est fermé.

Mesurez l'alignement de nuit avec au moins deux étoiles connues réparties sur l'image :

1. Choisissez une zone de ciel dégagée avec une étoile reconnaissable près du centre et une autre près du bord. Réglez le lieu et l'heure corrects, puis allumez la caméra avec **Cam : 5**. Une boussole calibrée donne la direction de départ la plus fiable.
2. Tenez l'appareil immobile et appuyez sur **Cam : 6** pour entrer dans **Aligner**. Utilisez les flèches pour pousser le ciel dessiné vers l'étoile photographiée jusqu'à ce que l'étoile centrale coïncide : droite déplace le dessin vers la droite, gauche vers la gauche, et haut/bas dans leur direction. Chaque appui change le décalage vers la droite ou vers le haut de l'écran de 0.5°. La ligne d'état affiche les décalages.
3. Gardez l'étoile centrale en coïncidence. Appuyez sur **Cam : 7** pour resserrer ou sur **Cam : 8** pour élargir le champ de l'objectif par pas de 1°, jusqu'à ce que l'étoile près du bord coïncide aussi. Revérifiez le centre et recommencez si besoin.
4. Appuyez de nouveau sur **Cam : 6** quand la touche 6 affiche la coche (**Terminé**). Si vous avez changé une valeur, cela enregistre le champ et les décalages de visée vers la droite et vers le haut de l'écran dans `~/.config/zerosky/camera-alignment` (ou `$XDG_CONFIG_HOME/zerosky/camera-alignment`). Ces décalages s'appliquent seulement quand la caméra est allumée. **Terminé** sans changement ne marque pas le champ estimé comme mesuré.

Pour annuler des changements non terminés, éteignez la caméra avec **Cam : 5**, ou quittez l'application avec ESC pendant qu'Aligner est ouvert. Aucune de ces deux actions n'enregistre l'alignement non terminé. Une fois un alignement enregistré, `FOV estimé` disparaît de la ligne d'état de la caméra.

L'alignement de la caméra est indépendant de la correspondance des repères de l'IMU et de la calibration magnétique. Il corrige le champ et le centre de l'image photographiée. Il ne corrige pas une horloge, un lieu ou une référence du nord incorrects. `ZEROSKY_CAMERA_FOV` remplace le champ enregistré pour une exécution contrôlée, laissez-la donc non définie pendant la mesure. L'alignement optique et la précision de visée de nuit n'ont pas encore été vérifiés sur l'appareil.

## Réglages de vue enregistrés

Après un redémarrage, ZeroSky restaure le thème nuit/jour choisi, le filtrage diurne Dim/All, l'état allumé/éteint de la caméra et le zoom de Ciel ordinaire. Ces choix sont stockés dans `~/.config/zerosky/settings` (ou `$XDG_CONFIG_HOME/zerosky/settings`). Le zoom de Ciel est enregistré quand vous quittez l'application, pas à chaque appui sur une touche de zoom. L'alignement de l'objectif a son propre fichier, comme décrit plus haut. Le choix du matériel magnétique reste dans `~/.config/zerosky/hardware` et la langue dans `~/.config/zeroradio/lang`. Les variables d'environnement comme `ZEROSKY_THEME`, `ZEROSKY_DAYLIGHT`, `ZEROSKY_CAMERA`, `ZEROSKY_CAMERA_FOV` et `ZEROSKY_HARDWARE` ont priorité quand elles sont fournies au démarrage. Si la caméra a été enregistrée comme allumée, la capture peut démarrer automatiquement au lancement suivant.

## Langue

L'interface, l'aide intégrée et ce manuel existent en anglais, italien, français, allemand, espagnol, japonais et chinois simplifié. Le texte japonais et chinois utilise une police CJK de secours du système.

Appuyez sur **Rég : 5** pour choisir la langue. Chaque appui passe au choix suivant : Auto, EN, IT, FR, DE, ES, JA, ZH, puis de nouveau Auto. La touche affiche le code à deux lettres, ou une icône de traduction pour Auto. Un message confirme le changement. Redémarrez ZeroSky pour l'appliquer : l'interface garde l'ancienne langue jusque-là. Auto suit la locale du système.

Le choix est enregistré dans `~/.config/zeroradio/lang` (ou `$XDG_CONFIG_HOME/zeroradio/lang`). Ce fichier est prévu pour toute la suite ZeroRadio, mais pour l'instant seul ZeroSky le lit. Au démarrage, ZeroSky lit d'abord `ZERORADIO_LANG`, puis ce fichier, puis `LC_ALL`, `LC_MESSAGES` et `LANG`. Les deux premières lettres choisissent la traduction. Une langue non prise en charge revient à l'anglais. Le QR code de la page À propos ouvre ce manuel dans la langue choisie.

## Lieu

Appuyez sur **Ciel : 5** pour ouvrir Lieu. Tapez le nom d'une ville de la liste hors ligne, ou des coordonnées comme `36.05, 14.24` (latitude positive au nord, longitude positive à l'est). Déplacez-vous avec haut/bas et appuyez sur Enter sur la ligne voulue. Un récepteur GPS USB ou le Cap LoRa-1262-GPS peut proposer une position. La ligne GPS n'est sélectionnable qu'une fois qu'il a une position. Vous pouvez toujours utiliser une ville ou des coordonnées saisies sans matériel GNSS.

ZeroSky enregistre la position choisie dans le fichier de lieu partagé avec ZeroRadio, pour que les deux applications utilisent le même endroit. La source du lieu et le réglage du matériel magnétique sont indépendants : choisir une ville ne signifie pas que le cap ou le GPS USB est absent. La position, la date actuelle et WMM2025 déterminent la référence magnétique utilisée par la boussole.

## Chercher un objet

Appuyez sur **Ciel : 8** pour ouvrir Chercher. Tapez une partie d'un nom ou d'une désignation de catalogue, puis choisissez une ligne et appuyez sur Enter. La liste comprend le Soleil, la Lune, les planètes, les étoiles nommées et les objets Messier, NGC et IC fournis. Les objets sous l'horizon sont signalés. Pendant le guidage, 8 affiche un signe stop : appuyez dessus pour arrêter le guidage et les bips. Si la cible est à l'écran, un anneau la marque. Si elle est hors du champ, une flèche au bord indique de quel côté tourner, et l'étiquette donne la séparation angulaire. Une cible sous l'horizon peut être choisie mais ne produit aucun bip. Sinon, le signal sonore devient plus rapide et plus aigu à mesure que vous vous approchez. À moins d'environ 1°, il devient continu. Le son demande un périphérique audio fonctionnel.

La recherche utilise les positions calculées par l'application. Elle n'est pas plus précise que l'horloge de l'appareil, le lieu, la correspondance des repères et la calibration magnétique. Pour une cible choisie au démarrage pendant une démonstration, `ZEROSKY_TARGET` peut nommer un objet pris en charge.

## Viser avec l'IMU

Le BMI270 mesure l'accélération et la rotation. Le BMM150 fournit une direction magnétique. ZeroSky les combine avec un filtre de Mahony et utilise le World Magnetic Model 2025 pour rapporter le cap au nord vrai. Les axes des capteurs de l'appareil et l'axe représenté par le centre de l'écran doivent être mesurés sur l'exemplaire physique. Une matrice identité d'usine ne prouve pas l'alignement.

Avec une correspondance des repères valide mais sans calibration ou référence magnétique utilisable, l'état est `inclinaison seule`. La hauteur et le roulis suivent l'IMU. L'azimut suit le gyroscope depuis une direction de départ arbitraire et peut dériver. Visez un objet ou un repère connu et utilisez les flèches gauche/droite pour aligner la vue. Les flèches haut/bas ne pilotent pas la vue quand l'IMU est active. TAB passe en commande entièrement manuelle. Dès que la configuration matérielle choisie a une calibration valide faite en extérieur et que la boussole fonctionne correctement, ZeroSky utilise le champ magnétique pour le pilotage vers le nord vrai et `inclinaison seule` disparaît.

La configuration utilise deux fichiers distincts dans `~/.config/zerosky/` (ou `$XDG_CONFIG_HOME/zerosky/`) :

- `frame-mapping.json` enregistre les rotations vérifiées de l'accéléromètre, du gyroscope et du magnétomètre, l'axe de visée et l'axe du haut de l'écran. Enregistrez le schéma 2 avec `zerosky_probe --save-frames` et les cinq options de repère, après avoir mesuré les axes. Les fichiers existants au schéma 1 se chargent toujours. S'ils n'ont pas d'axe magnétique, l'application peut reprendre le `sensor_to_body` mesuré d'un profil magnétique existant.
- `magnetometer-calibration.json` stocke un ajustement hard-iron/soft-iron fait en extérieur pour chaque configuration matérielle montée. Enregistrez-le avec `zerosky_probe --capture none`, `--capture cap` ou `--capture usb_gps`, en utilisant la position actuelle, la date UTC et une rotation du magnétomètre vérifiée.

La sonde installée est `/usr/share/zerosky/bin/zerosky_probe`. Lancez-la avec `--help` pour voir ses options, et suivez [docs/probe.md](probe.md) pour la procédure de mesure. Ne copiez pas les matrices de rotation d'exemple du guide sauf si elles correspondent aux axes mesurés sur votre appareil. La sonde refuse un ajustement sans couverture tridimensionnelle suffisante et conserve un profil valide existant si le nouvel ajustement échoue.

Pour la calibration magnétique dans l'application, réglez d'abord une position et choisissez le profil du matériel réellement branché avec **Rég : 6**. En extérieur, loin du métal, appuyez sur l'icône de boussole de **Rég : 8**. Suivez le grand 8 animé tout en tournant l'appareil sur toutes ses faces, y compris face vers le bas. Un 8 tracé à plat ne couvre pas la sphère. Huit points montrent les octants couverts, et la barre montre les échantillons indépendants et le temps restant. La capture s'arrête à 200 échantillons indépendants avec au moins 15 dans chaque octant, ou après 120 secondes. Deux tons montants annoncent un ajustement réussi. Un ton grave signale un échec. L'ajustement vérifie la couverture tridimensionnelle et le résidu sur des échantillons mis de côté. Une grande coche apparaît alors avec le RMS et le champ WMM attendu. Appuyez sur Enter pour enregistrer le profil choisi et le recharger aussitôt, ou sur Esc pour annuler sans rien écrire. Un ajustement raté donne une raison, et le profil précédent est conservé. Si aucun repère magnétique mesuré n'est disponible, ni au schéma 2 ni dans un profil antérieur, enregistrez d'abord les axes avec la sonde.

Sur la page **Rég**, la touche 6 fait défiler `none` (un panneau d'interdiction), `cap` (une antenne) et `usb_gps` (une prise USB). Choisissez la configuration physique réellement branchée, même quand la position vient d'une ville enregistrée. Le choix est conservé dans `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` impose un choix au démarrage de l'application pour les exécutions contrôlées. Un profil manquant pour la configuration choisie n'est jamais remplacé en silence par un autre. L'application utilise `inclinaison seule` tant que l'IMU est disponible. La détection automatique des accessoires n'est pas encore activée.

## Messages d'état

| Message | Signification et action |
| --- | --- |
| `manuel` | La caméra est commandée par les touches. TAB peut activer le pilotage par l'IMU quand elle est disponible. |
| `axes non mesurés` | Aucune correspondance des repères valide n'a été chargée. Vérifiez les axes avec la sonde et enregistrez la correspondance. |
| `fichier d'axes invalide` | La correspondance des repères enregistrée n'a pas passé la validation. Refaites la vérification des repères et enregistrez un fichier valide. |
| `pas d'IMU` ou `l'IMU ne démarre pas` | L'application n'a pas pu détecter ou démarrer les capteurs. La commande manuelle reste disponible. |
| `inclinaison seule` | La hauteur et le roulis suivent l'IMU. Le cap du gyroscope a un nord arbitraire et peut dériver. Alignez-le avec les flèches gauche/droite. Calibrez la configuration matérielle choisie en extérieur pour le pilotage vers le nord vrai. |
| `boussole perturbée` | Le champ mesuré ne correspond pas au champ attendu. Le pilotage par l'IMU peut continuer avec un cap peu fiable. Éloignez-vous du métal ou des appareils électroniques proches, ou appuyez sur TAB pour passer en commande manuelle. |

L'application met à jour les positions dans le ciel à intervalles réguliers. Une date ou un lieu inexacts déplacent les objets prévus même quand la boussole fonctionne correctement. Une calibration faite à côté d'une rambarde métallique peut aussi passer la validation du fichier tout en donnant un mauvais cap ailleurs. Confirmez-la en extérieur avec une référence indépendante, céleste ou géographique.

## Données, licences et limites actuelles

Le catalogue d'étoiles contient 5 070 étoiles HYG v4.1 jusqu'à la magnitude 6. L'application applique la précession à leurs positions J2000 pour la date d'observation, mais n'applique pas encore le mouvement propre de chaque étoile. Une sélection dérivée d'OpenNGC ajoute 605 objets Messier, NGC et IC. Astronomy Engine calcule le Soleil, la Lune et les planètes. WMM2025 fournit hors ligne la déclinaison magnétique et l'intensité du champ attendue. La liste des villes GeoNames est aussi hors ligne.

Le code de ZeroSky est sous licence MIT. Les fichiers dérivés d'étoiles HYG et d'objets du ciel profond OpenNGC sont sous CC BY-SA 4.0. Les lignes des constellations occidentales portent les mentions d3-celestial BSD-3-Clause et IAU / *Sky & Telescope* CC BY 4.0. Le code et les coefficients du WMM sont du domaine public du gouvernement des États-Unis. Consultez [CREDITS.md](../CREDITS.md) et les [sources des données](data-sources.md) pour les révisions et les attributions.

Le fond caméra et le haut-parleur fonctionnent sur le CardputerZero. Le profil magnétique `cap` a été calibré en extérieur, et le Soleil a servi à une première vérification de l'alignement. Les autres profils matériels, l'alignement de l'objectif et la précision de visée de nuit doivent encore être validés. La pose longue et le plate solving ne sont pas implémentés. Le [README](../README.md) donne les instructions de compilation et l'état du développement.
