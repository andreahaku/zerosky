# ZeroSky-Benutzerhandbuch

ZeroSky ist ein Taschenplanetarium für den M5Stack CardputerZero. Es zeichnet den Himmel für die aktuelle Uhrzeit des Geräts und Ihren gewählten Standort, mit rotem Nachtmodus und blauem Tagmodus. Die IMU kann die Ansicht schon vor der Magnetkalibrierung steuern, mit einer beliebigen Nordrichtung. Nach der Kalibrierung liefert der Kompass eine Richtung bezogen auf geografisch Nord. Sie können den Himmel auch mit den Tasten erkunden oder das Live-Kamerabild unter dem berechneten Himmel anzeigen.

## Erste Schritte

1. Öffnen Sie ZeroSky im Launcher des CardputerZero. Beim ersten Start erscheint der Dialog Standort. Wählen Sie eine Stadt, geben Sie Koordinaten ein oder warten Sie auf eine GPS-Position, und drücken Sie dann Enter.
2. Prüfen Sie die Uhr des Geräts. Die Himmelspositionen verwenden die Systemzeit. Die Wahl einer GPS-Position stellt die Uhr nicht.
3. Erkunden Sie den Himmel manuell mit den Pfeiltasten oder mit F/X/Z/C. Drücken Sie 4, um die Seiten Himmel, Ansicht, Kam und Einst der unteren Leiste zu durchlaufen.
4. Damit die Ansicht dem Gerät folgt, messen und speichern Sie seine Sensorachsen. Ohne Magnetkalibrierung folgt `nur Neigung` der Höhe und dem Rollwinkel, und die Pfeiltasten links/rechts richten die Blickrichtung aus. Kalibrieren Sie das Magnetometer im Freien, um die Steuerung nach geografisch Nord zu aktivieren. Die [Anleitung zum Messprogramm](probe.md) nennt die Befehle.

## Den Himmel lesen

Die Mitte des Displays ist die aktuelle Zielrichtung. Der Azimut wird im Uhrzeigersinn von geografisch Nord gemessen: 0° Nord, 90° Ost, 180° Süd und 270° West. Die Höhe ist 0° am Horizont und darüber positiv. Die Statuszeile zeigt `Az`, `Alt`, das horizontale `FOV` und den gewählten Standort.

Die Größe der Sterne richtet sich nach ihrer Helligkeit. Linien verbinden die westlichen Sternbildfiguren. Sie dienen der Veranschaulichung und sind keine offiziellen IAU-Grenzen. Ihre lateinischen Namen stehen an Beschriftungspunkten aus d3-celestial. Deep-Sky-Objekte haben Kartensymbole und Katalognamen. Sonne, Mond und Planeten sind beschriftet. Der dunkle Boden und die Horizontlinie zeigen, wo der Himmel auf die lokale Horizontalebene trifft. Das Display verwendet berechnete Richtungen und einen einfachen Horizont. Nahe Gebäude und Gelände werden nicht berücksichtigt.

Bei aktiver Tageslichtfilterung ändert sich die Sterngrenze stufenlos, während die Sonne durch die bürgerliche, nautische und astronomische Dämmerung sinkt. Das ist eine Anzeigehilfe, keine Messung der örtlichen Himmelshelligkeit. Wolken, Stadtlicht und Mondlicht können verändern, was Sie tatsächlich sehen. Schalten Sie die Tageslichtfilterung auf der Seite Ansicht aus, um den Nachtkatalog am Tag anzuzeigen. Sternbildlinien und Objekte des Sonnensystems werden weiterhin gemäß der aktuellen Ansicht gezeichnet.

## Tasten

| Taste | Seite Himmel | Seite Ansicht | Seite Kam | Seite Einst |
| --- | --- | --- | --- | --- |
| 4 | Nächste Seite, halten für die vorherige | Nächste Seite, halten für die vorherige | Nächste Seite, halten für die vorherige | Nächste Seite, halten für die vorherige |
| 5 | Standort öffnen | Zwischen Nacht- und Tagmodus wechseln | Kamerahintergrund an/aus | Sprache wählen |
| 6 | Verkleinern (Kamera aus) | Tageslichtfilterung ändern: halber Stern (Dim) oder ganzer Stern (All) | Ausrichten/Fertig | Magnetische Hardware `none`, `cap` oder `usb_gps` wählen |
| 7 | Vergrößern (Kamera aus) | Sternbilder wechseln: aus, Linien, Linien mit lateinischen Namen | Bildfeld des Objektivs in Ausrichten verengen | Über, QR-Code zum Handbuch und Credits öffnen |
| 8 | Suche öffnen (Lupe); während der Führung beenden (Stoppzeichen) | Deep-Sky-Symbole und Namen ein-/ausblenden | Bildfeld des Objektivs in Ausrichten erweitern | Kompasskalibrierung öffnen |

Außerhalb von Ausrichten drehen die Pfeiltasten die manuelle Ansicht um 5° pro Druck. In `nur Neigung` stellen die Pfeiltasten links/rechts den Richtungsversatz ein. Höhe und Rollwinkel folgen der IMU. Auf dem CardputerZero wirken F/X/Z/C außerhalb eines Textfelds auch als oben/unten/links/rechts. TAB wechselt auf dem Gerät zwischen IMU-Steuerung und manueller Steuerung. Der Desktop-Simulator ist immer manuell. H öffnet eine kürzere Hilfeseite. ESC schließt einen offenen Dialog oder eine Hilfeseite. Ist nichts davon offen, beendet ESC die App. In der Hilfe und in Über blättern F/X, und Z/C wechseln die Seite, wo vorhanden.

Die Seite Kam zeigt für **5** ein Kamerasymbol, wenn das Bild aus ist, und eine durchgestrichene Kamera, wenn es an ist. **6** zeigt ein Fadenkreuz für Ausrichten und ein Häkchen für Fertig. Die Tastennummern bleiben gleich, auch wenn sich das Symbol ändert.

Auf der Seite Ansicht zeigt **7** ein durchgestrichenes Auge, wenn die Sternbilder ausgeblendet sind, verbundene Linien für die Figuren allein und eine Sternliste für Figuren mit lateinischen Namen. **8** zeigt Kreise, wenn Deep-Sky-Symbole und Namen an sind, und einen gestrichelten Kreis, wenn sie aus sind. Namen und Symbole können entfallen, wo sie eine andere Beschriftung bedrängen würden. Standardmäßig sind Sternbildlinien mit Namen und Deep-Sky-Objekte an. Beide Einstellungen bleiben in `~/.config/zerosky/settings` gespeichert. Die Liste von Suche und ihre Führung enthalten Deep-Sky-Objekte auch dann, wenn ihre Anzeige aus ist.

Bei ausgeschalteter Kamera reicht das horizontale Bildfeld von 15° bis 120°. Ein engeres Bildfeld vergrößert den Himmel und zeigt schwächere Katalogsterne, wenn die Tageslichtfilterung sie zulässt. Die Zoomtasten der Seite Himmel sind gesperrt, solange die Kamera an ist. Der Himmelszoom wird beim Verlassen der App gespeichert, nicht nach jedem Tastendruck.

## Kamera und Objektivausrichtung

Drücken Sie auf der Seite **Kam** die Taste **5**, um das IMX219-Bild hinter den berechneten Sternen, Sternbildlinien, dem Horizont und der Führung anzuzeigen. Das erste Bild kann einige Sekunden dauern. Linien und Beschriftungen haben eine Kontur oder einen dunklen Hintergrund, damit sie über einer hellen Szene lesbar bleiben. Fehlt die Kamera oder veraltet ihr Bild, bleibt der berechnete Himmelshintergrund nutzbar. Drücken Sie **5** erneut, um die Kamera freizugeben. Solange sie an ist, ist der Himmelszoom gesperrt und die Ansicht verwendet stattdessen das Bildfeld des Objektivs. Der Startwert von 62.2° horizontal ist ein Schätzwert für die Pi Camera v2, keine Messung an diesem Gerät. Bis Sie eine Ausrichtung speichern, zeigt die Statuszeile `FOV geschätzt`, wenn die Kamera an und Ausrichten geschlossen ist.

Messen Sie die Ausrichtung nachts mit zwei oder mehr bekannten Sternen, die über das Bild verteilt sind:

1. Wählen Sie einen freien Himmelsausschnitt mit einem erkennbaren Stern nahe der Mitte und einem weiteren nahe dem Rand. Stellen Sie den richtigen Standort und die richtige Uhrzeit ein und schalten Sie dann mit **Kam: 5** die Kamera ein. Ein kalibrierter Kompass liefert die zuverlässigste Startrichtung.
2. Halten Sie das Gerät ruhig und drücken Sie **Kam: 6**, um **Ausrichten** zu starten. Schieben Sie mit den Pfeiltasten den gezeichneten Himmel zum fotografierten Stern, bis der mittlere Stern übereinstimmt: rechts bewegt die Zeichnung nach rechts, links nach links, oben/unten in ihre Richtung. Jeder Druck ändert den Versatz nach rechts oder nach oben auf dem Bildschirm um 0.5°. Die Statuszeile zeigt die Versätze.
3. Halten Sie den mittleren Stern in Deckung. Drücken Sie **Kam: 7** zum Verengen oder **Kam: 8** zum Erweitern des Bildfelds des Objektivs in Schritten von 1°, bis auch der Stern nahe dem Rand übereinstimmt. Prüfen Sie die Mitte erneut und wiederholen Sie bei Bedarf.
4. Drücken Sie erneut **Kam: 6**, wenn Taste 6 das Häkchen zeigt (**Fertig**). Haben Sie einen Wert geändert, speichert das das Bildfeld und die Zielversätze nach rechts/oben in `~/.config/zerosky/camera-alignment` (oder `$XDG_CONFIG_HOME/zerosky/camera-alignment`). Diese Versätze gelten nur, solange die Kamera an ist. **Fertig** ohne Änderungen markiert das geschätzte Bildfeld nicht als gemessen.

Um nicht abgeschlossene Änderungen zu verwerfen, schalten Sie die Kamera mit **Kam: 5** aus oder verlassen Sie die App mit ESC, während Ausrichten offen ist. Keine der beiden Aktionen speichert die unfertige Ausrichtung. Sobald eine Ausrichtung gespeichert ist, verschwindet `FOV geschätzt` aus der Statuszeile der Kamera.

Die Kameraausrichtung ist unabhängig von der Rahmenzuordnung der IMU und der Magnetkalibrierung. Sie korrigiert Bildfeld und Mitte des fotografierten Bildes. Eine falsche Uhr, einen falschen Standort oder eine falsche Nordreferenz korrigiert sie nicht. `ZEROSKY_CAMERA_FOV` überschreibt das gespeicherte Bildfeld für einen kontrollierten Lauf. Lassen Sie die Variable beim Messen also ungesetzt. Die optische Ausrichtung und die nächtliche Zielgenauigkeit sind auf dem Gerät noch nicht überprüft.

## Gespeicherte Ansichtseinstellungen

ZeroSky stellt nach einem Neustart den gewählten Nacht-/Tagmodus, die Tageslichtfilterung Dim/All, Kamera an/aus und den normalen Himmelszoom wieder her. Diese Einstellungen liegen in `~/.config/zerosky/settings` (oder `$XDG_CONFIG_HOME/zerosky/settings`). Der Himmelszoom wird beim Verlassen der App gespeichert, nicht bei jedem Druck auf eine Zoomtaste. Die Objektivausrichtung der Kamera hat eine eigene Datei, wie oben beschrieben. Die Wahl der magnetischen Hardware liegt in `~/.config/zerosky/hardware`, die Sprache in `~/.config/zeroradio/lang`. Umgebungsvariablen wie `ZEROSKY_THEME`, `ZEROSKY_DAYLIGHT`, `ZEROSKY_CAMERA`, `ZEROSKY_CAMERA_FOV` und `ZEROSKY_HARDWARE` haben Vorrang, wenn sie beim Start gesetzt sind. Ist die Kamera als an gespeichert, kann die Aufnahme beim nächsten Start automatisch beginnen.

## Sprache

Die Oberfläche, die Hilfe in der App und dieses Handbuch gibt es auf Englisch, Italienisch, Französisch, Deutsch, Spanisch, Japanisch und vereinfachtem Chinesisch. Japanischer und chinesischer Text verwendet eine CJK-Ersatzschrift des Systems.

Drücken Sie **Einst: 5**, um die Sprache zu wählen. Jeder Druck springt zur nächsten Option: Auto, EN, IT, FR, DE, ES, JA, ZH, dann wieder Auto. Die Taste zeigt den zweistelligen Code oder für Auto ein Übersetzungssymbol. Ein Hinweis bestätigt die Änderung. Starten Sie ZeroSky neu, um sie anzuwenden: Bis dahin behält die Oberfläche die alte Sprache. Auto folgt der Systemsprache.

Die Wahl wird in `~/.config/zeroradio/lang` (oder `$XDG_CONFIG_HOME/zeroradio/lang`) gespeichert. Die Datei ist für die gesamte ZeroRadio-Suite gedacht, derzeit liest sie aber nur ZeroSky. Beim Start liest ZeroSky zuerst `ZERORADIO_LANG`, dann diese Datei, dann `LC_ALL`, `LC_MESSAGES` und `LANG`. Die ersten beiden Buchstaben wählen die Übersetzung. Eine nicht unterstützte Sprache fällt auf Englisch zurück. Der QR-Code auf der Seite Über öffnet dieses Handbuch in der gewählten Sprache.

## Standort

Drücken Sie **Himmel: 5**, um Standort zu öffnen. Tippen Sie einen Stadtnamen aus der Offline-Städteliste oder Koordinaten wie `36.05, 14.24` (Breite nach Norden positiv, Länge nach Osten positiv). Bewegen Sie sich mit oben/unten und drücken Sie Enter auf der gewünschten Zeile. Ein USB-GPS-Empfänger oder der Cap LoRa-1262-GPS kann eine Position liefern. Die GPS-Zeile ist erst wählbar, wenn sie eine Position hat. Ohne GNSS-Hardware können Sie weiterhin eine Stadt oder eingegebene Koordinaten verwenden.

ZeroSky speichert die gewählte Position in der Standortdatei, die es mit ZeroRadio teilt. So können beide Apps denselben Ort verwenden. Die Standortquelle und die Einstellung der magnetischen Hardware sind unabhängig: Die Wahl einer Stadt bedeutet nicht, dass der Cap oder das USB-GPS fehlt. Position, aktuelles Datum und WMM2025 bestimmen die magnetische Referenz, die der Kompass verwendet.

## Ein Objekt suchen

Drücken Sie **Himmel: 8**, um Suche zu öffnen. Tippen Sie einen Teil eines Namens oder einer Katalogbezeichnung, wählen Sie dann eine Zeile und drücken Sie Enter. Die Liste enthält Sonne, Mond, Planeten, benannte Sterne und die mitgelieferten Messier-, NGC- und IC-Objekte. Objekte unter dem Horizont sind markiert. Während der Führung zeigt 8 ein Stoppzeichen: Drücken Sie es, um Führung und Pieptöne zu beenden. Ist das Ziel auf dem Bildschirm, markiert es ein Ring. Liegt es außerhalb des Bildfelds, zeigt ein Pfeil am Rand die Drehrichtung, und die Beschriftung nennt den Winkelabstand. Ein Ziel unter dem Horizont lässt sich wählen, gibt aber keinen Ton. Sonst wird das akustische Signal schneller und höher, je näher Sie kommen. Innerhalb von etwa 1° wird es zum Dauerton. Der Ton braucht ein funktionierendes Audiogerät.

Die Suche verwendet die von der App berechneten Positionen. Sie ist nur so genau wie Uhr, Standort, Rahmenzuordnung und Magnetkalibrierung des Geräts. Für ein beim Start gewähltes Ziel während einer Vorführung kann `ZEROSKY_TARGET` ein unterstütztes Objekt benennen.

## Zielen mit der IMU

Der BMI270 misst Beschleunigung und Drehung. Der BMM150 liefert eine magnetische Richtung. ZeroSky kombiniert beide mit einem Mahony-Filter und verwendet das World Magnetic Model 2025, um die Richtung auf geografisch Nord zu beziehen. Die Sensorachsen des Geräts und die Achse, die die Bildschirmmitte darstellt, müssen am physischen Gerät gemessen werden. Eine Einheitsmatrix ab Werk ist kein Nachweis der Ausrichtung.

Mit einer gültigen Rahmenzuordnung, aber ohne nutzbare Magnetkalibrierung oder Referenz, lautet der Status `nur Neigung`. Höhe und Rollwinkel folgen der IMU. Der Azimut folgt dem Gyroskop ab einer beliebigen Startrichtung und kann driften. Zielen Sie auf ein bekanntes Objekt oder einen Orientierungspunkt und richten Sie die Ansicht mit den Pfeiltasten links/rechts aus. Die Pfeiltasten oben/unten steuern nicht, solange die IMU aktiv ist. TAB wechselt zur vollständig manuellen Steuerung. Sobald die gewählte Hardwarekonfiguration eine gültige Kalibrierung im Freien hat und der Kompass einwandfrei arbeitet, verwendet ZeroSky das Magnetfeld für die Steuerung nach geografisch Nord, und `nur Neigung` verschwindet.

Die Einrichtung verwendet zwei getrennte Dateien unter `~/.config/zerosky/` (oder `$XDG_CONFIG_HOME/zerosky/`):

- `frame-mapping.json` enthält die überprüften Rotationen von Beschleunigungssensor, Gyroskop und Magnetometer, die Zielachse und die Achse zur Oberkante des Bildschirms. Speichern Sie Schema 2 mit `zerosky_probe --save-frames` und allen fünf Rahmenoptionen, nachdem Sie die Achsen gemessen haben. Vorhandene Dateien mit Schema 1 werden weiter geladen. Fehlt ihnen eine Magnetachse, kann die App das gemessene `sensor_to_body` aus einem vorhandenen Magnetprofil übernehmen.
- `magnetometer-calibration.json` speichert für jede montierte Hardwarekonfiguration eine im Freien ermittelte Hard-Iron-/Soft-Iron-Anpassung. Zeichnen Sie sie mit `zerosky_probe --capture none`, `--capture cap` oder `--capture usb_gps` auf, mit aktueller Position, UTC-Datum und einer überprüften Magnetometerrotation.

Das installierte Messprogramm ist `/usr/share/zerosky/bin/zerosky_probe`. Rufen Sie es mit `--help` auf, um seine Optionen zu sehen, und folgen Sie [docs/probe.md](probe.md) für das Messverfahren. Kopieren Sie keine Beispiel-Rotationsmatrizen aus der Anleitung, es sei denn, sie stimmen mit den an Ihrem Gerät gemessenen Achsen überein. Das Messprogramm lehnt eine Anpassung ohne ausreichende dreidimensionale Abdeckung ab und behält ein vorhandenes gültiges Profil, wenn die neue Anpassung fehlschlägt.

Für die Magnetkalibrierung in der App legen Sie zuerst eine Position fest und wählen mit **Einst: 6** das Profil der tatsächlich angeschlossenen Hardware. Drücken Sie im Freien, fern von Metall, das Kompasssymbol auf **Einst: 8**. Folgen Sie der großen bewegten Acht und drehen Sie das Gerät dabei über alle Seiten, auch mit dem Bildschirm nach unten. Eine flache Acht allein deckt die Kugel nicht ab. Acht Punkte zeigen die abgedeckten Oktanten. Der Balken zeigt die unabhängigen Messwerte und die verbleibende Zeit. Die Aufnahme endet bei 200 unabhängigen Messwerten mit mindestens 15 in jedem Oktanten oder nach 120 Sekunden. Zwei steigende Töne melden eine erfolgreiche Anpassung, ein tiefer Ton einen Fehlschlag. Die Anpassung prüft die dreidimensionale Abdeckung und das Residuum auf zurückgehaltenen Messwerten. Danach erscheint ein großes Häkchen mit RMS und erwartetem WMM-Feld. Drücken Sie Enter, um das gewählte Profil zu speichern und sofort neu zu laden, oder Esc, um ohne Schreiben abzubrechen. Eine fehlgeschlagene Anpassung nennt einen Grund, und das vorherige Profil bleibt erhalten. Gibt es weder aus Schema 2 noch aus einem früheren Profil einen gemessenen Magnetrahmen, speichern Sie zuerst die Achsen mit dem Messprogramm.

Auf der Seite **Einst** wechselt Taste 6 zwischen `none` (ein Verbotszeichen), `cap` (eine Antenne) und `usb_gps` (ein USB-Stecker). Wählen Sie die physische Konfiguration, die tatsächlich angeschlossen ist, auch wenn die Position aus einer gespeicherten Stadt stammt. Die Auswahl liegt in `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` wählt beim Start der App eine Überschreibung für kontrollierte Läufe. Fehlt das Profil für die gewählte Konfiguration, ersetzt die App es nie stillschweigend durch ein anderes. Sie verwendet `nur Neigung`, solange die IMU verfügbar ist. Die automatische Erkennung von Zubehör ist noch nicht aktiviert.

## Statusmeldungen

| Meldung | Bedeutung und Maßnahme |
| --- | --- |
| `manuell` | Die Kamera wird über die Tasten gesteuert. TAB kann die IMU-Steuerung aktivieren, wenn sie verfügbar ist. |
| `Achsen nicht gemessen` | Es wurde keine gültige Rahmenzuordnung geladen. Prüfen Sie die Achsen mit dem Messprogramm und speichern Sie die Zuordnung. |
| `Achsendatei ungültig` | Die gespeicherte Rahmenzuordnung hat die Prüfung nicht bestanden. Wiederholen Sie die Rahmenprüfung und speichern Sie eine gültige Datei. |
| `keine IMU` oder `IMU startet nicht` | Die App konnte die Sensoren nicht erkennen oder starten. Die manuelle Steuerung bleibt verfügbar. |
| `nur Neigung` | Höhe und Rollwinkel folgen der IMU. Die Gyroskop-Richtung hat ein beliebiges Nord und kann driften. Richten Sie sie mit den Pfeiltasten links/rechts aus. Kalibrieren Sie die gewählte Hardwarekonfiguration im Freien für die Steuerung nach geografisch Nord. |
| `Kompass gestört` | Das gemessene Feld weicht vom erwarteten Feld ab. Die IMU-Steuerung kann mit unzuverlässiger Richtung weiterlaufen. Entfernen Sie sich von nahem Metall oder Elektronik, oder drücken Sie TAB für die manuelle Steuerung. |

Die App aktualisiert die Himmelspositionen regelmäßig. Ein ungenaues Datum oder ein ungenauer Standort verschiebt die vorhergesagten Objekte, auch wenn der Kompass einwandfrei arbeitet. Eine Kalibrierung neben einem Metallgeländer kann ebenfalls die Dateiprüfung bestehen und anderswo trotzdem eine schlechte Richtung liefern. Prüfen Sie sie im Freien an einer unabhängigen Himmels- oder geografischen Referenz.

## Daten, Lizenzen und aktuelle Grenzen

Der Sternkatalog enthält 5.070 Sterne aus HYG v4.1 bis zur Größenklasse 6. Die App rechnet ihre J2000-Positionen mit der Präzession auf das Beobachtungsdatum um, berücksichtigt aber noch nicht die Eigenbewegung einzelner Sterne. Eine abgeleitete Auswahl aus OpenNGC fügt 605 Messier-, NGC- und IC-Objekte hinzu. Astronomy Engine berechnet Sonne, Mond und Planeten. WMM2025 liefert offline die magnetische Deklination und die erwartete Feldstärke. Die GeoNames-Städteliste ist ebenfalls offline.

Der Code von ZeroSky steht unter der MIT-Lizenz. Die abgeleiteten HYG-Stern- und OpenNGC-Deep-Sky-Dateien stehen unter CC BY-SA 4.0. Die westlichen Sternbildlinien tragen Hinweise zu d3-celestial BSD-3-Clause und IAU / *Sky & Telescope* CC BY 4.0. Code und Koeffizienten des WMM sind gemeinfreies Material der US-Regierung. Revisionen und Quellenangaben finden Sie in [CREDITS.md](../CREDITS.md) und [Datenquellen](data-sources.md).

Kamerahintergrund und Lautsprecher funktionieren auf dem CardputerZero. Das Magnetprofil `cap` wurde im Freien kalibriert, und die Sonne diente für eine erste Prüfung der Ausrichtung. Andere Hardwareprofile, die Objektivausrichtung und die nächtliche Zielgenauigkeit müssen noch validiert werden. Langzeitbelichtung und Plate Solving sind nicht implementiert. Das [README](../README.md) enthält Build-Anleitungen und den Entwicklungsstand.
