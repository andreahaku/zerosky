# Manuale utente di ZeroSky

ZeroSky è un planetario tascabile per M5Stack CardputerZero. Disegna il cielo per l'ora attuale del dispositivo e per la località che hai selezionato, con un tema notte rosso e un tema giorno blu. La IMU può guidare la vista anche prima della calibrazione magnetica, con una direzione del nord arbitraria. Dopo la calibrazione, la bussola fornisce una direzione riferita al nord vero. Puoi anche esplorare con i tasti o mostrare l'immagine della camera dal vivo sotto il cielo calcolato.

## Per iniziare

1. Apri ZeroSky dal launcher del CardputerZero. Al primo utilizzo compare la finestra Posizione. Scegli una città, inserisci le coordinate o attendi un fix GPS, poi premi Enter.
2. Controlla l'orologio del dispositivo. Le posizioni nel cielo usano l'ora di sistema. Selezionare una località GPS non imposta l'orologio.
3. Usa le frecce, o F/X/Z/C, per esplorare in manuale. Premi 4 per passare in ciclo tra le pagine Cielo, Vista, Cam e Imp della barra in basso.
4. Per far seguire la vista al dispositivo, misura e salva gli assi del suo sensore. Senza calibrazione magnetica, `solo inclinazione` segue altezza e rollio, mentre le frecce sinistra/destra allineano la direzione. Calibra il magnetometro all'aperto per attivare la guida riferita al nord vero. La [guida alla sonda di campo](probe.md) riporta i comandi.

## Leggere il cielo

Il centro del display è la direzione di puntamento attuale. L'azimut si misura in senso orario dal nord vero: 0° nord, 90° est, 180° sud e 270° ovest. L'altezza è 0° sull'orizzonte e positiva sopra di esso. La riga di stato mostra `Az`, `Alt`, il `FOV` orizzontale e la località selezionata.

La dimensione delle stelle dipende dalla luminosità. Le linee uniscono le figure delle costellazioni occidentali. Sono illustrative e non corrispondono ai confini ufficiali IAU. I loro nomi latini compaiono nei punti di etichetta di d3-celestial. Gli oggetti del profondo cielo hanno simboli da carta celeste e nomi di catalogo. Il Sole, la Luna e i pianeti hanno un'etichetta. Il suolo scuro e la linea dell'orizzonte mostrano dove il cielo incontra il piano orizzontale locale. Il display usa direzioni calcolate e un orizzonte semplice: edifici e rilievi vicini non sono rappresentati.

Con il filtro della luce del giorno attivo, il limite delle stelle cambia gradualmente mentre il Sole scende attraverso il crepuscolo civile, nautico e astronomico. È un aiuto di visualizzazione, non una misura della luminosità locale del cielo. Nuvole, luci della città e luce lunare possono cambiare quello che vedi davvero. Disattiva il filtro della luce del giorno nella pagina Vista per mostrare di giorno il catalogo notturno. Le linee delle costellazioni e gli oggetti del Sistema solare restano disegnati secondo la vista attuale.

## Tasti

| Tasto | Pagina Cielo | Pagina Vista | Pagina Cam | Pagina Imp |
| --- | --- | --- | --- | --- |
| 4 | Pagina successiva; tieni premuto per la precedente | Pagina successiva; tieni premuto per la precedente | Pagina successiva; tieni premuto per la precedente | Pagina successiva; tieni premuto per la precedente |
| 5 | Apre Posizione | Passa dal tema notte al tema giorno e viceversa | Sfondo della camera acceso/spento | Sceglie la lingua |
| 6 | Riduce lo zoom (camera spenta) | Cambia il filtro della luce del giorno: mezza stella (attenuato) o stella piena (tutte) | Allinea/Fine | Sceglie l'hardware magnetico `none`, `cap` o `usb_gps` |
| 7 | Aumenta lo zoom (camera spenta) | Cambia le costellazioni: nascoste, linee, linee con i nomi latini | Stringe il FOV della lente in Allinea | Apre Informazioni, QR code del manuale e Crediti |
| 8 | Apre Trova (lente d'ingrandimento); durante la guida, ferma (segno di stop) | Mostra/nasconde simboli e nomi del profondo cielo | Allarga il FOV della lente in Allinea | Apre la calibrazione della bussola |

Fuori da Allinea, le frecce ruotano la vista manuale di 5° a ogni pressione. In `solo inclinazione`, le frecce sinistra/destra regolano lo scostamento della direzione. Altezza e rollio seguono la IMU. Sul CardputerZero, fuori da un campo di testo, F/X/Z/C fanno anche da su/giù/sinistra/destra. Sul dispositivo, TAB passa dalla guida con la IMU al controllo manuale e viceversa. Il simulatore desktop è sempre in manuale. H apre una pagina di guida più breve. ESC chiude una finestra o una pagina di guida aperta, oppure esce dall'app quando non c'è nulla aperto. Nella guida e in Informazioni, F/X scorrono il testo e Z/C cambiano pagina dove disponibile.

La pagina Cam usa per **5** l'icona di una camera quando l'immagine è spenta e una camera barrata quando è accesa. **6** mostra un mirino per Allinea e un segno di spunta per Fine. I numeri dei tasti restano gli stessi anche quando l'icona cambia.

Nella pagina Vista, **7** mostra un occhio barrato quando le costellazioni sono nascoste, linee collegate per le sole figure e un elenco di stelle per le figure con i nomi latini. **8** mostra dei cerchi quando simboli e nomi del profondo cielo sono attivi, oppure un cerchio tratteggiato quando sono spenti. Nomi e simboli possono essere omessi dove si sovrapporrebbero a un'altra etichetta. Per impostazione predefinita sono attive le linee delle costellazioni con i nomi e il profondo cielo. Entrambe le scelte restano salvate in `~/.config/zerosky/settings`. L'elenco di Trova e la sua guida includono gli oggetti del profondo cielo anche quando la loro visualizzazione è spenta.

Con la camera spenta, il campo visivo orizzontale va da 15° a 120°. Un campo più stretto ingrandisce il cielo e mostra stelle più deboli del catalogo, quando il filtro della luce del giorno lo consente. I tasti di zoom di Cielo sono disattivati mentre la camera è accesa. Lo zoom di Cielo viene salvato quando esci dall'app, non dopo ogni pressione di tasto.

## Camera e allineamento della lente

Nella pagina **Cam**, premi **5** per mostrare l'immagine della IMX219 dietro le stelle calcolate, le linee delle costellazioni, l'orizzonte e la guida. La prima immagine può richiedere qualche secondo. Linee ed etichette hanno un contorno o uno sfondo scuro, così restano leggibili sopra una scena luminosa. Se la camera manca o il suo fotogramma smette di aggiornarsi, lo sfondo del cielo calcolato resta utilizzabile. Premi di nuovo **5** per rilasciare la camera. Mentre è accesa, lo zoom di Cielo è disattivato e la vista usa invece il FOV della lente. Il valore iniziale, 62.2° in orizzontale, è una stima ricavata dalla Pi Camera v2, non una misura di questo esemplare. Finché non salvi un allineamento, la riga di stato mostra `FOV stimato` quando la camera è accesa e Allinea è chiuso.

Misura l'allineamento di notte, con due o più stelle note distribuite nell'immagine:

1. Scegli una zona di cielo libera con una stella riconoscibile vicino al centro e un'altra vicino al bordo. Imposta la località e l'ora corrette, poi accendi la camera con **Cam: 5**. Una bussola calibrata dà la direzione di partenza più affidabile.
2. Tieni fermo il dispositivo e premi **Cam: 6** per entrare in **Allinea**. Usa le frecce per spingere il cielo disegnato verso la stella fotografata, finché la stella centrale coincide: destra sposta il disegno a destra, sinistra lo sposta a sinistra, e su/giù lo spostano nella loro direzione. Ogni pressione cambia di 0.5° lo scostamento verso destra o verso l'alto dello schermo. La riga di stato mostra gli scostamenti.
3. Mantieni coincidente la stella centrale. Premi **Cam: 7** per stringere o **Cam: 8** per allargare il FOV della lente a passi di 1°, finché coincide anche la stella vicino al bordo. Ricontrolla il centro e ripeti se serve.
4. Premi di nuovo **Cam: 6** quando il tasto 6 mostra il segno di spunta (**Fine**). Se hai cambiato un valore, questo salva il FOV e gli scostamenti di puntamento verso destra e verso l'alto dello schermo in `~/.config/zerosky/camera-alignment` (o `$XDG_CONFIG_HOME/zerosky/camera-alignment`). Questi scostamenti valgono solo mentre la camera è accesa. **Fine** senza modifiche non segna come misurato il FOV stimato.

Per scartare le modifiche non concluse, spegni la camera con **Cam: 5**, oppure esci dall'app con ESC mentre Allinea è aperto. Nessuna delle due azioni salva l'allineamento non concluso. Dopo che hai salvato un allineamento, `FOV stimato` scompare dalla riga di stato della camera.

L'allineamento della camera è separato dalla mappatura del sistema di riferimento della IMU e dalla calibrazione magnetica. Corregge il campo e il centro dell'immagine fotografata. Non corregge un orologio, una località o un riferimento del nord sbagliati. `ZEROSKY_CAMERA_FOV` ha la precedenza sul FOV salvato per un'esecuzione controllata, quindi lascialo non impostato mentre misuri. L'allineamento ottico e la precisione di puntamento notturna non sono ancora stati verificati sul dispositivo.

## Impostazioni della vista salvate

Dopo un riavvio, ZeroSky ripristina il tema notte/giorno selezionato, il filtro della luce del giorno (attenuato o tutte), la camera accesa/spenta e il normale zoom di Cielo. Queste scelte stanno in `~/.config/zerosky/settings` (o `$XDG_CONFIG_HOME/zerosky/settings`). Lo zoom di Cielo viene salvato quando esci dall'app, non a ogni pressione dei tasti di zoom. L'allineamento della lente della camera ha un file proprio, come descritto sopra. La scelta dell'hardware magnetico resta in `~/.config/zerosky/hardware` e la lingua in `~/.config/zeroradio/lang`. Le variabili d'ambiente come `ZEROSKY_THEME`, `ZEROSKY_DAYLIGHT`, `ZEROSKY_CAMERA`, `ZEROSKY_CAMERA_FOV` e `ZEROSKY_HARDWARE` hanno la precedenza, se impostate all'avvio. Se hai salvato la camera accesa, la cattura può partire da sola all'avvio successivo.

## Lingua

L'interfaccia, la guida nell'app e questo manuale esistono in inglese, italiano, francese, tedesco, spagnolo, giapponese e cinese semplificato. Il testo giapponese e cinese usa un font CJK di sistema di riserva.

Premi **Imp: 5** per scegliere la lingua. Ogni pressione passa alla scelta successiva: Auto, EN, IT, FR, DE, ES, JA, ZH, poi di nuovo Auto. Il tasto mostra il codice di due lettere, oppure un'icona di traduzione per Auto. Un avviso conferma la modifica. Riavvia ZeroSky per applicarla: fino ad allora l'interfaccia mantiene la lingua precedente. Auto segue la lingua di sistema.

La scelta viene salvata in `~/.config/zeroradio/lang` (o `$XDG_CONFIG_HOME/zeroradio/lang`). Il file è pensato per tutta la suite ZeroRadio, ma per ora solo ZeroSky lo legge. All'avvio ZeroSky legge prima `ZERORADIO_LANG`, poi questo file, poi `LC_ALL`, `LC_MESSAGES` e `LANG`. Le prime due lettere scelgono la traduzione. Una lingua non supportata ricade sull'inglese. Il QR code della pagina Informazioni apre questo manuale nella lingua selezionata.

## Località

Premi **Cielo: 5** per aprire Posizione. Scrivi il nome di una città dall'elenco offline delle città, oppure coordinate come `36.05, 14.24` (latitudine positiva a nord, longitudine positiva a est). Muoviti con su/giù e premi Enter sulla riga desiderata. Un ricevitore GPS USB o il Cap LoRa-1262-GPS possono offrire un fix. La riga GPS è selezionabile solo dopo che ha una posizione. Senza hardware GNSS puoi comunque usare una città o coordinate scritte a mano.

ZeroSky salva la posizione selezionata nel file della località condiviso con ZeroRadio, così le due app possono usare lo stesso luogo. La fonte della località e l'impostazione dell'hardware magnetico sono indipendenti: scegliere una città non significa che il cap o il GPS USB siano assenti. Posizione, data attuale e WMM2025 determinano il riferimento magnetico usato dalla bussola.

## Trovare un oggetto

Premi **Cielo: 8** per aprire Trova. Scrivi parte di un nome o di una sigla di catalogo, poi seleziona una riga e premi Enter. L'elenco comprende il Sole, la Luna, i pianeti, le stelle con nome proprio e gli oggetti Messier, NGC e IC inclusi. Gli oggetti sotto l'orizzonte sono contrassegnati. Durante la guida, 8 mostra un segno di stop: premilo per fermare la guida e i bip. Se l'oggetto è sullo schermo, un anello lo indica. Se è fuori dal campo, una freccia sul bordo indica da che parte girarti e l'etichetta riporta la distanza angolare. Un oggetto sotto l'orizzonte si può selezionare, ma non emette bip. Negli altri casi il segnale sonoro diventa più rapido e più acuto man mano che ti avvicini. Entro circa 1° diventa continuo. Il suono richiede un dispositivo audio funzionante.

La ricerca usa le posizioni calcolate dall'app. La sua precisione dipende dall'orologio del dispositivo, dalla località, dalla mappatura del sistema di riferimento e dalla calibrazione magnetica. Per scegliere un oggetto all'avvio durante una dimostrazione, `ZEROSKY_TARGET` può indicare un oggetto supportato.

## Puntare con la IMU

Il BMI270 misura accelerazione e rotazione. Il BMM150 fornisce una direzione magnetica. ZeroSky li combina con un filtro Mahony e usa il World Magnetic Model 2025 per riferire la direzione al nord vero. Gli assi del sensore del dispositivo e l'asse rappresentato dal centro dello schermo vanno misurati sull'esemplare fisico. Una matrice identità di fabbrica non prova che gli assi siano allineati.

Con una mappatura del sistema di riferimento valida, ma senza una calibrazione o un riferimento magnetico utilizzabili, lo stato è `solo inclinazione`. Altezza e rollio seguono la IMU. L'azimut segue il giroscopio da una direzione di partenza arbitraria e può derivare. Inquadra un oggetto o un punto di riferimento noto e usa le frecce sinistra/destra per allineare la vista. Le frecce su/giù non guidano la vista mentre la IMU è attiva. TAB passa al controllo completamente manuale. Quando la configurazione hardware selezionata ha una calibrazione valida fatta all'aperto e la bussola funziona correttamente, ZeroSky usa il campo magnetico per la guida riferita al nord vero e `solo inclinazione` scompare.

La configurazione usa due file separati in `~/.config/zerosky/` (o `$XDG_CONFIG_HOME/zerosky/`):

- `frame-mapping.json` registra le rotazioni verificate di accelerometro, giroscopio e magnetometro, l'asse di puntamento e l'asse verso la parte alta dello schermo. Salva lo schema 2 con `zerosky_probe --save-frames` e tutte e cinque le opzioni del sistema di riferimento, dopo aver misurato gli assi. I file con schema 1 esistenti si caricano ancora. Se non hanno un asse magnetico, l'app può riusare il `sensor_to_body` misurato da un profilo magnetico esistente.
- `magnetometer-calibration.json` contiene una stima hard-iron/soft-iron fatta all'aperto per ogni configurazione hardware montata. Registrala con `zerosky_probe --capture none`, `--capture cap` o `--capture usb_gps`, usando la posizione attuale, la data UTC e una rotazione del magnetometro verificata.

La sonda installata è `/usr/share/zerosky/bin/zerosky_probe`. Avviala con `--help` per vedere le opzioni e segui [docs/probe.md](probe.md) per la procedura di misura. Non copiare le matrici di rotazione di esempio dalla guida, a meno che corrispondano agli assi misurati sul tuo dispositivo. La sonda rifiuta una stima senza una copertura tridimensionale sufficiente e conserva un profilo valido esistente se la nuova stima non riesce.

Per la calibrazione magnetica nell'app, imposta prima una posizione e seleziona con **Imp: 6** il profilo dell'hardware fisicamente collegato. All'aperto, lontano da metalli, premi l'icona della bussola su **Imp: 8**. Segui il grande 8 in movimento e intanto ruota il dispositivo su tutti i lati, anche a faccia in giù. Un 8 disegnato in piano non basta a coprire la sfera. Otto punti mostrano gli ottanti coperti. La barra mostra i campioni indipendenti e il tempo rimasto. La registrazione si ferma a 200 campioni indipendenti con almeno 15 in ogni ottante, oppure dopo 120 secondi. Due toni ascendenti annunciano una stima riuscita, un tono basso segnala un errore. La stima controlla la copertura tridimensionale e il residuo sui campioni tenuti da parte. Poi compare un grande segno di spunta con l'RMS e il campo WMM atteso. Premi Enter per salvare il profilo selezionato e ricaricarlo subito, oppure Esc per annullare senza scrivere nulla. Una stima non riuscita indica un motivo, e il profilo precedente resta. Se non è disponibile un sistema di riferimento magnetico misurato, né dallo schema 2 né da un profilo precedente, salva prima gli assi con la sonda.

Nella pagina **Imp**, il tasto 6 passa in ciclo tra `none` (un segnale di divieto), `cap` (un'antenna) e `usb_gps` (una spina USB). Scegli la configurazione fisica davvero collegata, anche quando la posizione viene da una città salvata. La selezione resta in `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` seleziona un valore alternativo all'avvio dell'app per le esecuzioni controllate. Se manca il profilo per la configurazione selezionata, l'app non lo sostituisce mai in silenzio con un altro: usa `solo inclinazione` finché la IMU è disponibile. Il rilevamento automatico degli accessori non è ancora attivo.

## Messaggi di stato

| Messaggio | Significato e azione |
| --- | --- |
| `manuale` | La camera è controllata dai tasti. TAB può attivare la guida con la IMU quando è disponibile. |
| `assi non misurati` | Non è stata caricata una mappatura del sistema di riferimento valida. Verifica gli assi con la sonda e salva la mappatura. |
| `file degli assi non valido` | La mappatura del sistema di riferimento salvata non ha superato la verifica. Ripeti il controllo degli assi e salva un file valido. |
| `nessuna IMU` o `la IMU non parte` | L'app non è riuscita a rilevare o avviare i sensori. Il controllo manuale resta disponibile. |
| `solo inclinazione` | Altezza e rollio seguono la IMU. La direzione del giroscopio ha un nord arbitrario e può derivare. Allineala con le frecce sinistra/destra. Calibra all'aperto la configurazione hardware selezionata per la guida riferita al nord vero. |
| `bussola disturbata` | Il campo misurato non corrisponde a quello atteso. La guida con la IMU può continuare con una direzione inaffidabile. Allontanati da metalli o apparecchi elettronici vicini, oppure premi TAB per il controllo manuale. |

L'app aggiorna periodicamente le posizioni nel cielo. Una data o una località imprecise spostano gli oggetti previsti anche quando la bussola funziona correttamente. Anche una calibrazione fatta accanto a una ringhiera di metallo può superare la verifica del file e dare poi una direzione scadente altrove. Confermala all'aperto con un riferimento celeste o geografico indipendente.

## Dati, licenze e limiti attuali

Il catalogo stellare contiene 5.070 stelle HYG v4.1 fino alla magnitudine 6. L'app applica la precessione alle loro posizioni J2000 fino alla data dell'osservazione, ma non applica ancora il moto proprio delle singole stelle. Una selezione derivata da OpenNGC aggiunge 605 oggetti Messier, NGC e IC. Astronomy Engine calcola il Sole, la Luna e i pianeti. WMM2025 fornisce offline la declinazione magnetica e l'intensità di campo attesa. Anche l'elenco delle città GeoNames è offline.

Il codice di ZeroSky è distribuito con licenza MIT. I file derivati delle stelle HYG e degli oggetti del cielo profondo OpenNGC sono CC BY-SA 4.0. Le linee delle costellazioni occidentali riportano le note d3-celestial BSD-3-Clause e IAU / *Sky & Telescope* CC BY 4.0. Il codice e i coefficienti WMM sono materiale di pubblico dominio del governo degli Stati Uniti. Consulta [CREDITS.md](../CREDITS.md) e le [fonti dei dati](data-sources.md) per revisioni e attribuzioni.

Lo sfondo della camera e l'altoparlante funzionano sul CardputerZero. Il profilo magnetico `cap` è stato calibrato all'aperto e il Sole è servito per un primo controllo dell'allineamento. Gli altri profili hardware, l'allineamento della lente e la precisione di puntamento notturna vanno ancora validati. Le lunghe esposizioni e il plate solving non sono implementati. Il [README](../README.md) contiene le istruzioni di compilazione e lo stato dello sviluppo.
