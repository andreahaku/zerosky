# Guida di ZeroSky

La vista del cielo segue la tua posizione e l'ora. Sul dispositivo può seguire anche la IMU. H apre questa pagina, F/X la scorrono ed Esc la chiude.

## Pagine e movimento

- 4 passa in ciclo tra Cielo, Vista, Cam e Imp. Tieni premuto 4 per la pagina precedente.
- Sul dispositivo, TAB passa dalla guida con la IMU alla camera manuale e viceversa. Sul desktop la camera è sempre manuale.
- Fuori da Allinea, le frecce spostano la vista manuale di 5 gradi. In modalità solo inclinazione, le frecce sinistra/destra allineano il nord arbitrario del giroscopio con il cielo reale. Su/giù seguono la IMU. Fuori dall'inserimento di testo, F/X/Z/C fanno anche da su/giù/sinistra/destra.

## Pagina Cielo

- 5 apre Posizione. Inserisci una città, una latitudine e una longitudine, oppure scegli un fix GPS. Usa su/giù ed Enter per selezionare. Posizione si apre da sola al primo utilizzo.
- 6 riduce lo zoom, 7 lo aumenta. Entrambi i tasti sono disattivati mentre la camera è accesa. Lo zoom di Cielo viene salvato quando esci dall'app.
- 8, indicato da una lente d'ingrandimento, apre Trova. Scrivi parte del nome di un oggetto, scegli una riga e premi Enter. Durante la guida, 8 mostra un segno di stop: premilo per fermare la guida e i bip. Un anello indica un oggetto sullo schermo. Una freccia sul bordo indica un oggetto fuori dalla vista. Bip più rapidi e più acuti indicano che sei più vicino.

## Pagina Vista

- 5 passa dal tema notte rosso al tema giorno blu e viceversa.
- 6 cambia il filtro della luce del giorno. Una mezza stella (attenuato) nasconde di giorno le stelle deboli. Una stella piena (tutte) le mostra.
- 7 cambia le figure delle costellazioni: nascoste, solo linee, poi linee con i nomi latini. L'icona passa da un occhio barrato a linee collegate e poi a un elenco di stelle.
- 8 mostra o nasconde i 605 simboli e nomi degli oggetti del profondo cielo. Trova può ancora localizzare un oggetto nascosto. Entrambe le scelte di Vista vengono salvate tra un avvio e l'altro.

## Pagina Cam

- 5, indicato da una camera o da una camera barrata, accende o spegne lo sfondo della camera dal vivo. Il primo fotogramma può richiedere qualche secondo. Linee, stelle ed etichette restano visibili sopra la foto.
- 6, indicato da un mirino o da un segno di spunta, entra in Allinea o lo conclude. Di notte, inquadra stelle che conosci. Porta una stella disegnata sulla stessa stella fotografata: destra sposta il disegno a destra, sinistra lo sposta a sinistra, e su/giù lo spostano allo stesso modo. Fai coincidere prima una stella vicino al centro, poi usa 7 e 8 per stringere o allargare il campo della lente finché coincide anche un'altra stella vicino a un bordo.
- In Allinea, ogni freccia cambia lo scostamento di puntamento di 0.5 gradi. 7 e 8 cambiano il campo della lente di 1 grado. Premi di nuovo 6 su **Fine** per salvare, solo se hai cambiato qualcosa. Se spegni la camera o esci con Esc, le modifiche di Allinea non concluse vanno perse.
- Finché non salvi un allineamento, la riga di stato mostra FOV stimato quando la camera è accesa fuori da Allinea. Il campo iniziale della lente di 62.2 gradi è una stima.
- Tema notte/giorno, filtro della luce del giorno, camera accesa/spenta e zoom di Cielo restano memorizzati tra un avvio e l'altro. Lo zoom di Cielo viene salvato all'uscita. **Fine** salva a parte l'allineamento della lente.

## Pagina Imp

- 5 sceglie la lingua: Auto, EN, IT, FR, DE, ES, JA, ZH. L'icona di traduzione indica Auto, che segue il sistema. Riavvia ZeroSky per applicare la nuova lingua.
- 6 cambia in ciclo il profilo dell'hardware magnetico: un segnale di divieto per none, un'antenna per cap, una spina USB per USB GPS. Usa la configurazione davvero collegata.
- 7 apre Informazioni, il QR code del manuale completo e Crediti.
- 8, indicato da una bussola, apre a schermo intero la calibrazione magnetica per il profilo hardware selezionato. All'aperto, lontano da metalli, muovi il dispositivo disegnando un 8 e giralo su ogni lato. Otto punti mostrano la copertura delle direzioni. La barra mostra l'avanzamento verso 200 campioni indipendenti e il tempo rimasto. La registrazione si ferma quando ogni ottante ha almeno 15 campioni, oppure dopo 120 secondi. Un segno di spunta e due toni ascendenti indicano che la stima è pronta: Enter salva, Esc annulla. Un tono basso e un motivo indicano che la stima non è riuscita.

## Lingua

- L'interfaccia, questa guida e il manuale esistono in inglese, italiano, francese, tedesco, spagnolo, giapponese e cinese semplificato. Un font CJK di sistema completa il testo giapponese e cinese.
- La pagina Imp salva la scelta in ~/.config/zeroradio/lang, un file per tutta la suite ZeroRadio. Per ora solo ZeroSky lo legge. ZERORADIO_LANG ha la precedenza sul file. Con Auto decidono LC_ALL, LC_MESSAGES e LANG.

## Riga di stato

- Az e Alt sono la direzione della vista. FOV è il campo visivo orizzontale. Segue il nome della località.
- manuale: la guida con la IMU è spenta e le frecce controllano la camera.
- assi non misurati: salva la mappatura del sistema di riferimento del dispositivo con zerosky_probe.
- nessuna IMU: il rilevamento del sensore non è riuscito.
- solo inclinazione: altezza e rollio seguono la IMU. L'azimut segue il giroscopio, ma il nord è arbitrario e può derivare. Usa le frecce sinistra/destra per allinearlo. Una calibrazione valida, fatta all'aperto per il profilo hardware selezionato, attiva la guida riferita al nord vero.
- bussola disturbata: il campo magnetico misurato non corrisponde a quello atteso. La IMU può ancora guidare la vista: allontanati da metalli o apparecchi elettronici, oppure premi TAB per il controllo manuale.

## Calibrazione

- Misura prima gli assi del sensore, compreso il magnetometro. Salvali con zerosky_probe --save-frames.
- Imposta una posizione, seleziona il profilo hardware con Imp: 6, poi premi la bussola su Imp: 8 all'aperto, lontano da metalli. Muovi il dispositivo disegnando un 8 e giralo su ogni lato. La finestra mostra i campioni distinti e gli ottanti coperti. Si ferma a copertura completa o dopo 120 secondi.
- Una stima riuscita mostra l'RMS e il campo WMM. Enter salva, Esc annulla senza cambiare il profilo precedente. Se la stima non riesce, segui il motivo indicato e ripeti. Il file docs/probe.md nel repository dei sorgenti descrive la procedura di misura degli assi.
