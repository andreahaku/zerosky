# Fase 1: sensori e orientamento

Specifica di progetto, 20 ottobre 2026. I numeri hardware sotto vengono dalla verifica sul CardputerZero di Andrea riportata nell'handover locale del 20 ottobre 2026; vanno riconfermati sul dispositivo prima di fissare gli assi. Questa fase produce un orientamento e una direzione di puntamento con qualità esplicita, indipendenti dalla UI e dal futuro flusso camera.

## 1. Contratto dei sistemi di riferimento

| Frame | Convenzione proposta | Stato |
| --- | --- | --- |
| `B`, corpo CardputerZero | Origine al centro del dispositivo; `+X` verso destra dello schermo, `+Y` verso il bordo superiore dello schermo, `+Z` uscente dal vetro verso chi lo guarda. Destro: `X × Y = Z`. | Convenzione software da adottare. |
| `A`, accelerometro BMI270 | Assi `in_accel_{x,y,z}_raw` del driver IIO. | Orientamento fisico rispetto a `B` da misurare. |
| `G`, giroscopio BMI270 | Assi `in_anglvel_{x,y,z}_raw` del driver IIO. | Verificare segno e allineamento con `A`; non presumere che i due canali siano già nel frame dello schermo. |
| `M`, magnetometro BMM150 | Assi `in_magn_{x,y,z}_raw` del driver IIO. | `in_mount_matrix` risultava identità sul device verificato, ma questo non dimostra l'allineamento fisico con `B`. |
| `C`, camera | `+Z_C` lungo la direzione di osservazione della lente, `+X_C` verso destra dell'immagine non specchiata, `+Y_C` verso il basso dell'immagine; `X_C × Y_C = Z_C`. | Estrinseca `R_BC` da misurare in fase 2. Nessuna ipotesi su quale faccia ospiti la lente. |
| `W`, mondo ENU | `+X` est, `+Y` nord geografico, `+Z` alto. Destro. | Frame pubblico del motore cielo. |

Per evitare l'ambiguità del frame ottico, nell'interfaccia della fase 1 basta conservare il **versore di puntamento** `u_B` e il versore `up_B` della vista. Si inizializzano da una verifica meccanica del dispositivo; in fase 2 si derivano dall'estrinseca camera. La direzione nel mondo è `u_W = R_WB u_B`. Non confondere il bordo superiore dello schermo con il nord del cielo.

Il filtro mantiene un quaternione unitario `q_WB` che ruota vettori dal corpo `B` a ENU `W`. Le rotazioni fisse `R_BA`, `R_BG`, `R_BM` portano ciascun sensore in `B`; sono matrici ortogonali destre, applicate **una sola volta**. Leggere la mount matrix IIO e documentare se è solo metadato o già applicata ai campioni dal driver, verificando il comportamento su questo kernel. Una matrice identità in sysfs non è una calibrazione. Per passare da NED (`nord, est, basso`), usato da WMM e da alcune librerie AHRS, a ENU: `(E,N,U) = (Y_NED,X_NED,-Z_NED)`. Se WMM restituisce `(X=nord,Y=est,Z=basso)` in nT, il vettore ENU è `(Y,X,-Z)` in nT. La declinazione `D = atan2(Y,X)` è positiva verso est.

### Lettura IIO

Scoprire BMI270 e BMM150 dal contenuto di `name` sotto `/sys/bus/iio/devices/iio:device*`, non dal numero `deviceN`. Leggere scala e disponibilità dei canali al bootstrap, poi campioni e tempi con clock monotono. L'[ABI IIO del kernel](https://code.googlesource.com/linux/torvalds/linux/+/556e2f6020bf90f63c5dd65e9a2254be6db3185b/Documentation/ABI/testing/sysfs-bus-iio) definisce accelerazione scalata in m/s², velocità angolare in rad/s e intensità magnetica in gauss: convertire quest'ultima in µT con `1 G = 100 µT` prima di calibrazione o confronto WMM. Sul device dell'handover: scala accelerometro `0.002394 m/s²/LSB`, giroscopio `0.001065 rad/s/LSB`, magnetometro `0.000625 G/LSB = 0.0625 µT/LSB`; sono osservazioni, non costanti di compilazione. Gestire anche eventuale `offset` IIO se esposto. Non mischiare campioni di tempi diversi come se fossero atomici.

## 2. Fusione e tempi

**Prima implementazione: filtro complementare di Mahony in quaternioni**, con correzione di gravità e campo magnetico separabili, bias del giroscopio stimato solo quando i riferimenti sono affidabili. Il [lavoro originale di Mahony, Hamel e Pflimlin](https://openresearch-repository.anu.edu.au/items/db91f02d-9a17-4ead-bc64-86ed5bc9c6f4) motiva l'osservatore e la stima del bias. [Madgwick](https://pubmed.ncbi.nlm.nih.gov/22275550/) è un'alternativa valida e leggera; per ZeroSky la separazione delle correzioni aiuta a disattivare il magnetometro disturbato senza perdere roll e pitch. Entrambi hanno costo aritmetico trascurabile rispetto al disegno LVGL su un Cortex-A53 a 1 GHz; il costo reale da misurare è polling sysfs, scheduling e latenza dell'intera pipeline, non dedurre millisecondi da stime teoriche.

- Configurazione iniziale: campionare accel e gyro a **100 Hz**, magnetometro a **20 Hz** se il driver lo consente, filtro a **100 Hz**, pubblicazione UI a **20-30 Hz**. Il BMI270 supporta ODR ben superiori; il [datasheet BMM150](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm150-ds001.pdf) e le capacità IIO del device vanno consultati per l'ODR effettivo. Il sistema osservato esponeva accel 100 Hz, gyro 200 Hz e magnetometro 2-30 Hz. Non superare la frequenza reale del canale leggendo più spesso lo stesso dato.
- Usare `dt` misurato con clock monotono, non `1/100` presunto. Rifiutare `dt <= 0` e i salti lunghi; dopo una pausa di circa 250 ms reinizializzare o propagare a passi limitati, senza un'unica integrazione grande.
- A riposo fare media del gyro per un bias iniziale. Correggere roll/pitch con accelerometro solo quando `|a|` è vicino a `g` e l'accelerazione è stabile; ridurre il peso durante movimenti bruschi. Il segno dell'accelerazione statica va verificato con il test «schermo in alto».
- Applicare la misura magnetica calibrata solo quando è nuova, recente (meno di circa 250 ms) e il controllo disturbi è superato. In caso contrario lavorare a sei assi: roll/pitch restano osservabili, yaw deriva nel tempo. Mostrare `heading_unreliable`, non un falso azimut assoluto. Al rientro del magnetometro, correggere yaw gradualmente per non far saltare la vista.
- Pubblicare `q_WB`, tempo monotono, età dell'ultimo campione, modalità 6/9 assi, qualità magnetica e stima del bias. Il rendering usa l'ultimo stato completo senza bloccare il thread LVGL. Il simulatore usa sequenze dichiarate simulate, con gli stessi contratti e stati di errore.

## 3. Calibrazione magnetica

La [calibrazione tramite ellissoide](https://www.nxp.com/docs/en/application-note/AN4246.pdf) stima l'offset hard-iron `b_M` e una matrice simmetrica positiva `S_M` per scale, non ortogonalità e soft-iron. Su campioni in µT nel frame `M`: `m_cal_M = S_M (m_raw_M - b_M)`, con norma attesa `F_WMM` in µT; poi `m_B = R_BM m_cal_M`. L'ellissoide non identifica da solo la rotazione rigida tra sensore e corpo, quindi `R_BM` richiede una verifica separata. Non adattare il profilo a un campo disturbato dell'ufficio.

Procedura di raccolta: all'aperto, lontano da ringhiere, moto, auto, cavi di potenza e oggetti ferromagnetici; configurazione d'uso reale e cap acceso se quel profilo lo prevede. Ruotare lentamente il dispositivo in molti assetti 3D, anche capovolto e su ciascun lato, per circa 60-120 secondi. Una sola rotazione piatta a otto non copre la sfera. Tenere campioni con timestamp e posizione, scartare outlier e ripetizioni; verificare copertura di tutti gli otto ottanti e distribuzione degli assi. Adattare un ellissoide vincolato positivo, ricavare `b_M` e `S_M`, scalare a `F_WMM`; rifiutare fit non positivo, mal condizionato o con residui alti. Come gate iniziali **da tarare**: almeno 200 campioni indipendenti, almeno 15 per ciascuno degli otto ottanti, rapporto tra assi dell'ellissoide < 4 e RMS di `||m_cal|| - F_WMM` < 5 µT su campioni tenuti fuori dal fit. Una calibrazione errata non sostituisce il profilo valido precedente.

Indicizzare i profili per **configurazione hardware magnetica**, inizialmente `none`, `cap` e `usb_gps`, con identificatori estendibili. La fonte delle coordinate è un dato distinto: città dal database offline, coordinate manuali, chiavetta GNSS USB o GNSS del cap possono alimentare WMM senza determinare da soli il profilo magnetico. Rilevare cap collegato/alimentato e dongle USB presente; se questa rilevazione non è affidabile, chiedere all'utente quale configurazione è montata. Non scegliere mai un profilo diverso in silenzio. Una cover, un accessorio magnetico o la corrente nel cap possono richiedere nuova calibrazione. Trattare `usb_gps` come configurazione distinta finché una misura sul dispositivo non dimostra che non altera il BMM150. Schema proposto in `~/.config/zerosky/magnetometer-calibration.json` (scrittura atomica con file temporaneo, rename e backup del precedente):

```json
{
  "schema": 1,
  "units": "uT",
  "sensor": "bmm150",
  "sensor_to_body": [[1, 0, 0], [0, 1, 0], [0, 0, 1]],
  "profiles": {
    "none": {
      "offset_uT": [0, 0, 0],
      "soft_iron": [[1, 0, 0], [0, 1, 0], [0, 0, 1]],
      "field_uT": 0,
      "rms_uT": 0,
      "samples": 0,
      "measured_at_utc": "",
      "location_deg": [0, 0]
    },
    "cap": null,
    "usb_gps": null
  }
}
```

I valori sono **segnaposto di schema**, non una calibrazione utilizzabile. Il loader rifiuta zero campioni, `field_uT <= 0`, matrici non finite o non definite positive, versione sconosciuta e profilo non corrispondente alla configurazione attuale. Se manca il profilo, pubblicare `unreliable` e invitare a calibrare. Salvare anche la revisione del modello WMM usato, la qualità del fit e possibilmente un identificativo della configurazione hardware.

## 4. Bussola disturbata

Calcolare `F_WMM` e declinazione WMM2025 per posizione, quota e data valide. Il [modello di errore NOAA](https://www.ncei.noaa.gov/products/world-magnetic-model/accuracy-limitations-error-model) dà in media circa 138 nT per `F` a una sigma, molto meno dei disturbi locali osservati; le soglie dell'app devono includere rumore del sensore, fit e materiali del dispositivo. Su ogni campione calibrato confrontare `F_meas = ||m_cal_M||` con `F_WMM` nella stessa unità.

Gate iniziale **da validare sul campo**: marcare disturbato se `|F_meas - F_WMM| > max(8 µT, 0.20 F_WMM)` per almeno 1 s; rientrare solo se lo scarto resta sotto `max(5 µT, 0.12 F_WMM)` per 3 s. L'isteresi evita lampeggiamenti. Il valore di circa 108 µT misurato in ufficio contro circa 44 µT attesi supererebbe chiaramente il gate, ma **non** distingue interferenza della stanza da quella del device. Il controllo della sola norma non rileva tutte le perturbazioni: aggiungere un gate sull'innovazione angolare tra campo misurato e previsione del filtro, e validare il heading contro un riferimento esterno. Se mancano posizione/data affidabili, profilo valido o magnetometro, segnalare `unknown`/`unreliable`; non dichiarare campo sano.

## 5. Validazione sul dispositivo

Prima delle prove reali, prendere il lock `device` nel duet. Andrea deve maneggiare fisicamente il CardputerZero all'aperto; non far partire una calibrazione automatica da remoto. La prova non usa la camera né alimenta il cap GNSS oltre il necessario. Se si accende `ext_5v_out`, registrare lo stato iniziale e ripristinarlo anche in caso di errore.

1. **Assi e unità, banco:** registrare `name`, mount matrix, scale, offset e frequenze IIO. A device fermo con schermo verso l'alto, controllare che `|a| ≈ 9.81 m/s²`; ripetere sulle sei facce. Ruotare a mano di circa 90° attorno agli assi di `B` per stabilire segni di gyro e matrici `R_BA`, `R_BG`. Portare un piccolo oggetto ferromagnetico a distanze controllate solo per testare la risposta del magnetometro, non per calibrare.
2. **Raccolta all'aperto:** scegliere un luogo aperto e lontano da metalli; registrare posizione e ora. Raccogliere separatamente i profili `none`, `cap` e `usb_gps`, con lo stesso montaggio e stato elettrico previsti nell'uso. Valutare copertura 3D, residui su dati non usati nel fit e ripetibilità in una seconda sessione. Confrontare `none` e `usb_gps` per quantificare l'effetto del dongle sulla bussola.
3. **Riferimento astronomico o geografico:** puntare il centro definito da `u_B` verso un riferimento con azimut e altezza indipendenti, come una stella/pianeta brillante calcolato da tempo e posizione oppure un punto distante con coordinate note. Evitare il solo nord letto da un'altra bussola non calibrata. Confrontare `R_WB u_B` con la direzione di riferimento in più azimut e inclinazioni; una sola direzione non separa errore di yaw ed errore dell'asse di puntamento.
4. **Dinamica e disturbi:** fare rotazioni lente e rapide, soste di 30 s e ritorni allo stesso target. Misurare latenza da movimento a display, errore angolare statico e dinamico, deriva in modalità sei assi e tempo di recupero dopo un disturbo magnetico. Ripetere con `none`, cap acceso/spento e dongle USB, poi vicino alla zona che in ufficio dava ~108 µT. Verificare che l'avviso compaia e che la vista non salti al recupero.
5. **Criteri iniziali di accettazione, da ritarare con Andrea:** errore statico mediano < 5° e percentile 95 < 10° all'aperto su target indipendenti; nessuna inversione di assi; nessun heading dichiarato affidabile sotto disturbo; aggiornamento UI regolare a 20-30 Hz. Salvare dati grezzi e risultati per riprodurre eventuali errori senza tornare subito sul posto.

I [datasheet Bosch BMI270](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf) e [BMM150](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm150-ds001.pdf) descrivono gli assi del chip, non il loro montaggio nel CardputerZero. Fino alla verifica dei sei orientamenti, le rotazioni sensore-corpo e l'asse di puntamento restano parametri non confermati.
