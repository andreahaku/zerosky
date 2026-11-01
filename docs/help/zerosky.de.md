# ZeroSky-Hilfe

Die Himmelsansicht folgt Ihrer Position und der Uhrzeit. Auf dem Gerät kann sie auch der IMU folgen. H öffnet diese Seite. F/X blättern, Esc schließt sie.

## Seiten und Bewegung

- 4 wechselt zwischen Himmel, Ansicht, Kam und Einst. Halten Sie 4 gedrückt für die vorherige Seite.
- Auf dem Gerät wechselt TAB zwischen IMU-Steuerung und manueller Kamera. Auf dem Desktop ist die Steuerung immer manuell.
- Außerhalb von Ausrichten bewegen die Pfeiltasten die manuelle Ansicht um 5 Grad. Im Modus nur Neigung richten die Pfeiltasten links/rechts den beliebigen Nordwert des Gyroskops am echten Himmel aus. Oben/unten folgen der IMU. Außerhalb der Texteingabe wirken F/X/Z/C auch als oben/unten/links/rechts.

## Seite Himmel

- 5 öffnet Standort. Geben Sie eine Stadt, Breite und Länge ein oder wählen Sie eine GPS-Position. Wählen Sie mit oben/unten und Enter aus. Standort öffnet sich beim ersten Start automatisch.
- 6 verkleinert, 7 vergrößert. Beide Tasten sind gesperrt, solange die Kamera an ist. Der Himmelszoom wird beim Verlassen der App gespeichert.
- 8, mit einer Lupe markiert, öffnet Suche. Tippen Sie einen Teil eines Objektnamens, wählen Sie eine Zeile und drücken Sie Enter. Während der Führung zeigt 8 ein Stoppzeichen: Drücken Sie es, um Führung und Pieptöne zu beenden. Ein Ring markiert ein Objekt auf dem Bildschirm. Ein Pfeil am Rand zeigt auf ein Objekt außerhalb der Ansicht. Schnellere, höhere Töne bedeuten: näher.

## Seite Ansicht

- 5 wechselt zwischen rotem Nachtmodus und blauem Tagmodus.
- 6 wechselt die Tageslichtfilterung. Ein halber Stern (Dim) blendet schwache Sterne bei Tag aus, ein ganzer Stern (All) zeigt sie.
- 7 wechselt die Sternbildfiguren: aus, nur Linien, dann Linien mit lateinischen Namen. Das Symbol wechselt von einem durchgestrichenen Auge zu verbundenen Linien und dann zu einer Sternliste.
- 8 blendet die 605 Deep-Sky-Symbole und Namen ein oder aus. Suche findet ein ausgeblendetes Objekt weiterhin. Beide Einstellungen der Seite Ansicht bleiben zwischen den Starts gespeichert.

## Seite Kam

- 5, mit einer Kamera oder einer durchgestrichenen Kamera markiert, schaltet den Kamerahintergrund an oder aus. Das erste Bild kann einige Sekunden dauern. Linien, Sterne und Beschriftungen bleiben über dem Foto sichtbar.
- 6, mit einem Fadenkreuz oder einem Häkchen markiert, startet oder beendet Ausrichten. Zielen Sie nachts auf bekannte Sterne. Schieben Sie einen gezeichneten Stern auf sein fotografiertes Gegenstück: rechts bewegt die Zeichnung nach rechts, links nach links, oben/unten entsprechend. Bringen Sie zuerst einen Stern nahe der Mitte zur Deckung. Verengen oder erweitern Sie dann mit 7 und 8 das Bildfeld des Objektivs, bis auch ein Stern nahe dem Rand übereinstimmt.
- In Ausrichten ändert jede Pfeiltaste den Zielversatz um 0.5 Grad. 7 und 8 ändern das Bildfeld des Objektivs um 1 Grad. Drücken Sie 6 erneut bei Fertig. Gespeichert wird nur, wenn Sie etwas geändert haben. Wenn Sie die Kamera ausschalten oder mit Esc verlassen, verwirft die App nicht abgeschlossene Änderungen in Ausrichten.
- Bis eine Ausrichtung gespeichert ist, zeigt die Statuszeile FOV geschätzt, wenn die Kamera außerhalb von Ausrichten an ist. Das anfängliche Bildfeld von 62.2 Grad ist ein Schätzwert.
- Nacht-/Tagmodus, Tageslichtfilterung, Kamera an/aus und Himmelszoom bleiben zwischen den Starts erhalten. Der Himmelszoom wird beim Beenden gespeichert. Fertig speichert die Objektivausrichtung getrennt.

## Seite Einst

- 5 wählt die Sprache: Auto, EN, IT, FR, DE, ES, JA, ZH. Das Übersetzungssymbol steht für Auto, das dem System folgt. Starten Sie ZeroSky neu, um die neue Sprache anzuwenden.
- 6 wechselt das magnetische Hardwareprofil: ein Verbotszeichen für none, eine Antenne für cap, ein USB-Stecker für USB GPS. Verwenden Sie die Konfiguration, die tatsächlich angeschlossen ist.
- 7 öffnet Über, den QR-Code zum vollständigen Handbuch und Credits.
- 8, mit einem Kompass markiert, öffnet die Magnetkalibrierung für das gewählte Hardwareprofil im Vollbild. Bewegen Sie das Gerät im Freien, fern von Metall, in einer Acht und drehen Sie dabei jede Seite. Acht Punkte zeigen die Abdeckung der Richtungen. Der Balken zeigt den Fortschritt bis 200 unabhängige Messwerte und die verbleibende Zeit. Die Aufnahme endet, wenn jeder Oktant mindestens 15 Messwerte hat, oder nach 120 Sekunden. Ein Häkchen und zwei steigende Töne bedeuten, dass die Anpassung bereit ist: Enter speichert, Esc bricht ab. Ein tiefer Ton und ein Grund bedeuten, dass die Anpassung fehlgeschlagen ist.

## Sprache

- Die Oberfläche, diese Hilfe und das Handbuch gibt es auf Englisch, Italienisch, Französisch, Deutsch, Spanisch, Japanisch und vereinfachtem Chinesisch. Eine CJK-Systemschrift ergänzt den japanischen und chinesischen Text.
- Die Seite Einst speichert die Wahl in ~/.config/zeroradio/lang, einer Datei für die gesamte ZeroRadio-Suite. Derzeit liest nur ZeroSky sie. ZERORADIO_LANG hat Vorrang. Mit Auto entscheiden LC_ALL, LC_MESSAGES und LANG.

## Statuszeile

- Az und Alt sind die Blickrichtung. FOV ist das horizontale Bildfeld. Danach folgt der Ortsname.
- manuell: Die IMU-Steuerung ist aus. Die Pfeiltasten steuern die Kamera.
- Achsen nicht gemessen: Speichern Sie die Zuordnung des Geräterahmens mit zerosky_probe.
- keine IMU: Die Sensorerkennung ist fehlgeschlagen.
- nur Neigung: Höhe und Rollwinkel folgen der IMU. Der Azimut folgt dem Gyroskop, aber Norden ist beliebig und kann driften. Richten Sie ihn mit den Pfeiltasten links/rechts aus. Eine gültige Kalibrierung im Freien für das gewählte Hardwareprofil ermöglicht die Steuerung nach geografisch Nord.
- Kompass gestört: Das gemessene Magnetfeld stimmt nicht mit dem erwarteten Feld überein. Die IMU kann weiter steuern. Entfernen Sie sich von Metall oder Elektronik, oder drücken Sie TAB für die manuelle Steuerung.

## Kalibrierung

- Messen Sie zuerst die Sensorachsen, einschließlich des Magnetometers. Speichern Sie sie mit zerosky_probe --save-frames.
- Legen Sie eine Position fest, wählen Sie das Hardwareprofil mit Einst: 6 und drücken Sie dann im Freien, fern von Metall, den Kompass auf Einst: 8. Bewegen Sie das Gerät in einer Acht und drehen Sie dabei jede Seite. Der Dialog zeigt verschiedene Messwerte und abgedeckte Oktanten. Er endet bei vollständiger Abdeckung oder nach 120 Sekunden.
- Eine erfolgreiche Anpassung zeigt RMS und das WMM-Feld. Enter speichert. Esc bricht ab, ohne das alte Profil zu ändern. Schlägt die Anpassung fehl, folgen Sie dem angezeigten Grund und wiederholen Sie den Vorgang. docs/probe.md im Quellrepository beschreibt das Messen der Achsen.
