/*
 * SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "i18n.h"

// Every text ZeroSky shows, in the toolkit languages (en, it, fr, de, es, ja,
// zh). toolkit::i18n::pick() chooses one from the locale, English as fallback.
// Key hints live next to their keys in sky_viewmodel.cpp.
namespace zerosky::text {

using toolkit::i18n::Text;

// NavBar pages.
inline constexpr Text kPageSky{"Sky", "Cielo", "Ciel", "Himmel", "Cielo", "空", "天空"};
inline constexpr Text kPageView{"View", "Vista", "Vue", "Ansicht", "Vista", "表示", "显示"};
inline constexpr Text kPageCam{"Cam", "Cam", "Cam", "Kam", "Cám", "カメラ", "相机"};
inline constexpr Text kPageSetup{"Set", "Imp", "Rég", "Einst", "Conf", "設定", "设置"};
inline constexpr Text kConstellationsOffHint{
    "Constellations hidden; press for lines", "Costellazioni nascoste; premi per le linee",
    "Constellations masquées ; appuyer pour les lignes",
    "Sternbilder aus; drücken für Linien", "Constelaciones ocultas; pulsa para líneas",
    "星座を非表示。押すと線を表示", "星座已隐藏；按下显示连线"};
inline constexpr Text kConstellationsLinesHint{
    "Constellation lines; press for Latin names", "Linee delle costellazioni; premi per i nomi latini",
    "Lignes des constellations ; appuyer pour les noms latins",
    "Sternbildlinien; drücken für lateinische Namen",
    "Líneas de constelaciones; pulsa para nombres latinos",
    "星座の線。押すとラテン語名を表示", "星座连线；按下显示拉丁名称"};
inline constexpr Text kConstellationsNamesHint{
    "Constellation lines and Latin names; press to hide",
    "Linee e nomi latini delle costellazioni; premi per nascondere",
    "Lignes et noms latins ; appuyer pour masquer",
    "Sternbildlinien und lateinische Namen; drücken zum Ausblenden",
    "Líneas y nombres latinos; pulsa para ocultar",
    "星座の線とラテン語名。押すと非表示", "星座连线与拉丁名称；按下隐藏"};
inline constexpr Text kDeepSkyOnHint{
    "Deep-sky symbols and names shown; press to hide",
    "Simboli e nomi del cielo profondo visibili; premi per nascondere",
    "Symboles et noms du ciel profond visibles ; appuyer pour masquer",
    "Deep-Sky-Symbole und Namen sichtbar; drücken zum Ausblenden",
    "Símbolos y nombres de cielo profundo visibles; pulsa para ocultar",
    "深空天体の記号と名前を表示中。押すと非表示",
    "深空天体符号与名称已显示；按下隐藏"};
inline constexpr Text kDeepSkyOffHint{
    "Deep sky hidden; Find still works; press to show",
    "Cielo profondo nascosto; Cerca funziona; premi per mostrare",
    "Ciel profond masqué ; Recherche fonctionne ; appuyer pour afficher",
    "Deep Sky aus; Suche funktioniert weiter; drücken zum Anzeigen",
    "Cielo profundo oculto; Buscar sigue activo; pulsa para mostrar",
    "深空天体を非表示。検索は有効。押すと表示",
    "深空天体已隐藏；搜索仍可用；按下显示"};
// Setup page: the note after a language change (it applies at the next start).
inline constexpr Text kLanguageRestart{
    "Language saved. Restart ZeroSky to apply it.",
    "Lingua salvata. Riavvia ZeroSky per applicarla.",
    "Langue enregistrée. Redémarrez ZeroSky pour l'appliquer.",
    "Sprache gespeichert. ZeroSky neu starten, um sie anzuwenden.",
    "Idioma guardado. Reinicia ZeroSky para aplicarlo.",
    "言語を保存しました。ZeroSky を再起動すると反映されます。",
    "语言已保存。重启 ZeroSky 后生效。"};

inline constexpr Text kCalibrationNavHint{
    "Calibrate the compass outdoors", "Calibra la bussola all'aperto",
    "Étalonner la boussole dehors", "Kompass draußen kalibrieren",
    "Calibrar la brújula al aire libre", "屋外でコンパスを調整", "在室外校准罗盘"};
inline constexpr Text kCalibrationTitle{
    "Compass calibration", "Calibrazione bussola", "Étalonnage boussole",
    "Kompasskalibrierung", "Calibración brújula", "コンパス調整", "罗盘校准"};
inline constexpr Text kCalibrationGuide{
    "Go outdoors, away from metal.\nMove in a figure 8 and turn every side.",
    "Vai all'aperto, lontano da metalli.\nMuovi a 8 e ruota su ogni lato.",
    "Dehors, loin du métal.\nFaites un 8 et tournez chaque face.",
    "Draußen, fern von Metall.\nAchterbewegung, alle Seiten drehen.",
    "Sal al exterior, lejos del metal.\nHaz un 8 y gira todas las caras.",
    "屋外で金属から離れる。\n8の字を描き、全方向に回す。",
    "到室外，远离金属。\n画8字并向各方向转动。"};
inline constexpr Text kCalibrationShortGuide{
    "Outside: turn a figure 8 on all sides", "Fuori: fai un 8 su tutti i lati",
    "Dehors : faites un 8 sur chaque face", "Draußen: 8 auf allen Seiten drehen",
    "Fuera: haz un 8 por todos los lados", "屋外で全方向に8の字を描く",
    "室外向各方向画8字"};
inline constexpr Text kCalibrationTurnAllSides{
    "Try: turn every side", "Prova: ruota ogni lato", "Essayez : tournez chaque face",
    "Tipp: alle Seiten drehen", "Prueba: gira todas las caras",
    "全方向に回してみて", "试着转动各个面"};
inline constexpr Text kCalibrationScreenDown{
    "Try: screen down", "Prova: schermo in basso", "Essayez : écran en bas",
    "Tipp: Display nach unten", "Prueba: pantalla abajo",
    "画面を下に向けて", "试着让屏幕朝下"};
inline constexpr Text kCalibrationScreenUp{
    "Try: screen up", "Prova: schermo in alto", "Essayez : écran en haut",
    "Tipp: Display nach oben", "Prueba: pantalla arriba",
    "画面を上に向けて", "试着让屏幕朝上"};
inline constexpr Text kCalibrationKeyboardDown{
    "Try: keyboard edge down", "Prova: tastiera in basso", "Essayez : clavier en bas",
    "Tipp: Tastatur nach unten", "Prueba: teclado abajo",
    "キーボード側を下に", "试着让键盘边朝下"};
inline constexpr Text kCalibrationTopDown{
    "Try: top edge down", "Prova: bordo alto in basso", "Essayez : bord haut en bas",
    "Tipp: Oberkante nach unten", "Prueba: borde superior abajo",
    "上端を下に向けて", "试着让上边缘朝下"};
inline constexpr Text kCalibrationRightDown{
    "Try: right edge down", "Prova: lato destro in basso", "Essayez : côté droit en bas",
    "Tipp: rechte Seite nach unten", "Prueba: lado derecho abajo",
    "右側を下に向けて", "试着让右边朝下"};
inline constexpr Text kCalibrationLeftDown{
    "Try: left edge down", "Prova: lato sinistro in basso", "Essayez : côté gauche en bas",
    "Tipp: linke Seite nach unten", "Prueba: lado izquierdo abajo",
    "左側を下に向けて", "试着让左边朝下"};
inline constexpr Text kCalibrationDoneTitle{
    "Calibration ready", "Calibrazione pronta", "Étalonnage prêt", "Kalibrierung bereit",
    "Calibración lista", "調整完了", "校准完成"};
inline constexpr Text kCalibrationFailedTitle{
    "Calibration failed", "Calibrazione fallita", "Étalonnage échoué", "Kalibrierung fehlgeschlagen",
    "Falló la calibración", "調整失敗", "校准失败"};
inline constexpr Text kCalibrationDemoHint{
    "Demo only. Esc closes. No save.",
    "Solo demo. Esc chiude. Non salva.",
    "Démo. Esc ferme. Sans sauvegarde.",
    "Nur Demo. Esc schließt. Kein Speichern.",
    "Solo demo. Esc cierra. No guarda.",
    "デモです。Escで閉じます。保存しません。",
    "仅演示。Esc关闭，不保存配置。"};
inline constexpr Text kCalibrationProgress{
    "%u/200    %ds", "%u/200    %ds", "%u/200    %ds", "%u/200    %ds",
    "%u/200    %ds", "%u/200    %d秒", "%u/200    %d秒"};
inline constexpr Text kCalibrationReady{
    "Fit OK  RMS %.1f uT  Field %.1f uT",
    "Fit OK  RMS %.1f uT  Campo %.1f uT",
    "Ajusté  RMS %.1f uT  Champ %.1f uT",
    "Fit OK  RMS %.1f uT  Feld %.1f uT",
    "Ajuste OK  RMS %.1f uT  Campo %.1f uT",
    "適合OK  RMS %.1f uT  磁場 %.1f uT",
    "拟合完成  RMS %.1f uT  磁场 %.1f uT"};
inline constexpr Text kCalibrationSaveHint{
    "Enter: save    Esc: cancel", "Invio: salva    Esc: annulla",
    "Entrée : sauver  Esc : annuler", "Enter: speichern  Esc: Abbruch",
    "Intro: guardar  Esc: cancelar", "Enter: 保存  Esc: 取消",
    "回车：保存  Esc：取消"};
inline constexpr Text kCalibrationCancelHint{
    "Esc: cancel without saving", "Esc: annulla senza salvare",
    "Esc : annuler sans sauver", "Esc: ohne Speichern abbrechen",
    "Esc: cancelar sin guardar", "Esc: 保存せず取消", "Esc：取消且不保存"};
inline constexpr Text kCalibrationSaved{
    "Saved. Esc closes.", "Salvato. Esc chiude.", "Enregistré. Esc ferme.",
    "Gespeichert. Esc schließt.", "Guardado. Esc cierra.",
    "保存しました。Escで閉じる。", "已保存。按Esc关闭。"};
inline constexpr Text kCalibrationReloadFailed{
    "Saved, but reload failed. Restart the app.",
    "Salvato, ma ricarica fallita. Riavvia l'app.",
    "Sauvé, rechargement échoué. Redémarrez.",
    "Gespeichert, Laden fehlgeschlagen. Neustart.",
    "Guardado, pero falló la recarga. Reinicia.",
    "保存済み、再読込失敗。再起動してください。",
    "已保存，但重新加载失败。请重启。"};
inline constexpr Text kCalibrationNoPosition{
    "Set a position first (Sky: 5).", "Prima imposta la posizione (Cielo: 5).",
    "Définir la position d'abord (Ciel : 5).", "Zuerst Position setzen (Himmel: 5).",
    "Primero fija la posición (Cielo: 5).", "先に位置を設定（空: 5）。",
    "先设置位置（天空: 5）。"};
inline constexpr Text kCalibrationNoFrame{
    "Axes unmeasured: run zerosky_probe.", "Assi non misurati: usa zerosky_probe.",
    "Axes non mesurés : lancez zerosky_probe.", "Achsen fehlen: zerosky_probe starten.",
    "Ejes sin medir: usa zerosky_probe.", "軸未測定：zerosky_probe を使用。",
    "轴未测量：运行 zerosky_probe。"};
inline constexpr Text kCalibrationNoMagFrame{
    "Mag axes missing: use probe --save-frames.", "Assi mag mancanti: probe --save-frames.",
    "Axes mag absents : probe --save-frames.", "Mag-Achsen fehlen: probe --save-frames.",
    "Faltan ejes mag: probe --save-frames.", "磁気軸なし：probe --save-frames。",
    "缺少磁轴：probe --save-frames。"};
inline constexpr Text kCalibrationNoMag{
    "BMM150 magnetometer not found.", "Magnetometro BMM150 assente.",
    "Magnétomètre BMM150 absent.", "BMM150-Magnetometer fehlt.",
    "Falta magnetómetro BMM150.", "BMM150磁気センサーなし。", "未找到BMM150磁力计。"};
inline constexpr Text kCalibrationNoWmm{
    "WMM field unavailable: check date/location.", "Campo WMM assente: controlla data/luogo.",
    "Champ WMM absent : vérifiez date/lieu.", "WMM-Feld fehlt: Datum/Ort prüfen.",
    "Sin campo WMM: comprueba fecha/lugar.", "WMM磁場なし：日時と位置を確認。",
    "无WMM磁场：检查日期和位置。"};
inline constexpr Text kCalibrationFew{
    "Need 200 distinct samples; move longer.", "Servono 200 campioni distinti: continua.",
    "Il faut 200 mesures distinctes.", "200 verschiedene Proben nötig.",
    "Se necesitan 200 muestras distintas.", "異なる標本が200個必要。",
    "需要200个不同样本。"};
inline constexpr Text kCalibrationOctants{
    "Cover all 8 directions; turn every side.", "Copri 8 direzioni: ruota ogni lato.",
    "Couvrez les 8 directions, tournez tout.", "Alle 8 Richtungen abdecken.",
    "Cubre las 8 direcciones; gira todo.", "8方向を覆う：全側面を回す。",
    "覆盖8个方向；转动各面。"};
inline constexpr Text kCalibrationDegenerate{
    "Poor 3D coverage; repeat all-side turns.", "Copertura 3D scarsa: ruota su ogni lato.",
    "Couverture 3D faible : tournez chaque face.", "3D-Abdeckung zu gering: alle Seiten drehen.",
    "Cobertura 3D escasa: gira todas las caras.", "3Dの範囲不足：全方向に回す。",
    "三维覆盖不足：转动各面。"};
inline constexpr Text kCalibrationShape{
    "Ellipsoid distorted; move away from metal.",
    "Ellissoide distorto: lontano da metalli.",
    "Ellipsoïde déformé : éloignez le métal.",
    "Ellipsoid verzerrt: weg von Metall.",
    "Elipsoide deformado: aléjate del metal.",
    "楕円体が歪む：金属から離れる。",
    "椭球变形：远离金属。"};
inline constexpr Text kCalibrationResidual{
    "Magnetic noise is too high; move away.", "Disturbo magnetico alto: allontanati.",
    "Bruit magnétique fort : éloignez-vous.", "Magnetisches Rauschen zu hoch: Abstand halten.",
    "Ruido magnético alto: aléjate.", "磁気ノイズが大きい：場所を変える。",
    "磁场噪声过大：换个位置。"};
inline constexpr Text kCalibrationInvalid{
    "Fit failed; retry outdoors.", "Fit fallito: riprova all'aperto.",
    "Ajustement échoué : réessayez dehors.", "Fit fehlgeschlagen: draußen erneut versuchen.",
    "Ajuste fallido: repite al aire libre.", "適合失敗：屋外で再試行。",
    "拟合失败：到室外重试。"};
inline constexpr Text kCalibrationSaveFailed{
    "Could not save the profile.", "Profilo non salvato.",
    "Profil non enregistré.", "Profil nicht gespeichert.",
    "No se pudo guardar el perfil.", "保存できませんでした。", "无法保存配置。"};

inline constexpr Text kLanguageNotSaved{
    "Could not save the language. The config folder is not writable.",
    "Impossibile salvare la lingua. La cartella di configurazione non è scrivibile.",
    "Impossible d'enregistrer la langue. Le dossier de configuration est protégé.",
    "Sprache nicht gespeichert. Der Konfigurationsordner ist schreibgeschützt.",
    "No se pudo guardar el idioma. La carpeta de configuración no admite escritura.",
    "言語を保存できません。設定フォルダーに書き込めません。",
    "无法保存语言。配置文件夹不可写。"};

// Status line.
inline constexpr Text kNoPosition{"no position: key 5", "nessuna posizione: tasto 5",
                                  "pas de position : touche 5", "keine Position: Taste 5",
                                  "sin posición: tecla 5", "位置なし：キー5", "无位置：按 5"};
inline constexpr Text kManual{"manual", "manuale", "manuel", "manuell", "manual", "手動", "手动"};
inline constexpr Text kTiltOnly{"tilt only", "solo inclinazione", "inclinaison seule", "nur Neigung",
                                "solo inclinación", "傾きのみ", "仅倾斜"};
inline constexpr Text kCompassDisturbed{"compass disturbed", "bussola disturbata",
                                        "boussole perturbée", "Kompass gestört",
                                        "brújula perturbada", "方位が乱れています", "罗盘受干扰"};
inline constexpr Text kAxesNotMeasured{"axes not measured", "assi non misurati", "axes non mesurés",
                                       "Achsen nicht gemessen", "ejes no medidos", "軸が未測定",
                                       "轴未测量"};
inline constexpr Text kNoImu{"no IMU", "nessuna IMU", "pas d'IMU", "keine IMU", "sin IMU", "IMUなし",
                             "无 IMU"};
inline constexpr Text kBadAxes{"bad axes file", "file degli assi non valido", "fichier d'axes invalide",
                               "Achsendatei ungültig", "archivo de ejes no válido", "軸ファイルが不正",
                               "轴文件无效"};
inline constexpr Text kImuFailed{"IMU did not start", "la IMU non parte", "l'IMU ne démarre pas",
                                 "IMU startet nicht", "la IMU no arranca", "IMUが起動しない",
                                 "IMU 无法启动"};
inline constexpr Text kNoWmm{"no WMM data", "dati WMM mancanti", "pas de données WMM",
                             "keine WMM-Daten", "sin datos WMM", "WMMデータなし", "无 WMM 数据"};
inline constexpr Text kFovEstimate{"FOV estimated", "FOV stimato", "FOV estimé", "FOV geschätzt",
                                   "FOV estimado", "画角は推定値", "视场为估计值"};
// ALIGN status, filled with the FOV and the two offsets in degrees.
inline constexpr Text kAlignStatus{"ALIGN  FOV %.1f  right %+.1f  up %+.1f",
                                   "ALLINEA  FOV %.1f  destra %+.1f  su %+.1f",
                                   "ALIGNER  FOV %.1f  droite %+.1f  haut %+.1f",
                                   "AUSRICHTEN  FOV %.1f  rechts %+.1f  oben %+.1f",
                                   "ALINEAR  FOV %.1f  derecha %+.1f  arriba %+.1f",
                                   "調整  画角 %.1f  右 %+.1f  上 %+.1f",
                                   "对齐  视场 %.1f  右 %+.1f  上 %+.1f"};

// Search guidance: "<name> set <n>°" when the target is below the horizon.
inline constexpr Text kSet{"set", "tramontato", "couché", "untergegangen", "puesto", "沈んだ", "已落"};
inline constexpr Text kSearchOff{"Search off", "Ricerca spenta", "Recherche arrêtée", "Suche aus",
                                 "Búsqueda apagada", "検索オフ", "搜索关闭"};

// Sun, Moon and planets, in sky::Body order.
inline constexpr Text kBodies[] = {
    {"Sun", "Sole", "Soleil", "Sonne", "Sol", "太陽", "太阳"},
    {"Moon", "Luna", "Lune", "Mond", "Luna", "月", "月球"},
    {"Mercury", "Mercurio", "Mercure", "Merkur", "Mercurio", "水星", "水星"},
    {"Venus", "Venere", "Vénus", "Venus", "Venus", "金星", "金星"},
    {"Mars", "Marte", "Mars", "Mars", "Marte", "火星", "火星"},
    {"Jupiter", "Giove", "Jupiter", "Jupiter", "Júpiter", "木星", "木星"},
    {"Saturn", "Saturno", "Saturne", "Saturn", "Saturno", "土星", "土星"},
    {"Uranus", "Urano", "Uranus", "Uranus", "Urano", "天王星", "天王星"},
    {"Neptune", "Nettuno", "Neptune", "Neptun", "Neptuno", "海王星", "海王星"},
};

// Compass points from north, every 45 degrees.
inline constexpr Text kCardinals[] = {
    {"N", "N", "N", "N", "N", "北", "北"},
    {"NE", "NE", "NE", "NO", "NE", "北東", "东北"},
    {"E", "E", "E", "O", "E", "東", "东"},
    {"SE", "SE", "SE", "SO", "SE", "南東", "东南"},
    {"S", "S", "S", "S", "S", "南", "南"},
    {"SW", "SO", "SO", "SW", "SO", "南西", "西南"},
    {"W", "O", "O", "W", "O", "西", "西"},
    {"NW", "NO", "NO", "NW", "NO", "北西", "西北"},
};

// Search list details.
inline constexpr Text kKindSun{"Sun", "Sole", "Soleil", "Sonne", "Sol", "太陽", "太阳"};
inline constexpr Text kKindMoon{"Moon", "Luna", "Lune", "Mond", "Luna", "月", "月球"};
inline constexpr Text kKindPlanet{"planet", "pianeta", "planète", "Planet", "planeta", "惑星", "行星"};
inline constexpr Text kKindStar{"star", "stella", "étoile", "Stern", "estrella", "恒星", "恒星"};
// In DeepSkyKind order.
inline constexpr Text kDeepSkyKinds[] = {
    {"galaxy", "galassia", "galaxie", "Galaxie", "galaxia", "銀河", "星系"},
    {"open cluster", "ammasso aperto", "amas ouvert", "offener Haufen", "cúmulo abierto", "散開星団",
     "疏散星团"},
    {"globular cluster", "ammasso globulare", "amas globulaire", "Kugelsternhaufen",
     "cúmulo globular", "球状星団", "球状星团"},
    {"nebula", "nebulosa", "nébuleuse", "Nebel", "nebulosa", "星雲", "星云"},
    {"planetary nebula", "nebulosa planetaria", "nébuleuse planétaire", "planetarischer Nebel",
     "nebulosa planetaria", "惑星状星雲", "行星状星云"},
    {"deep sky", "cielo profondo", "ciel profond", "Deep Sky", "cielo profundo", "深宇宙", "深空"},
};

// Camera start-up and Align steps.
inline constexpr Text kCameraStarting{"Starting the camera...", "Avvio della camera...",
                                      "Démarrage de la caméra...", "Kamera wird gestartet...",
                                      "Iniciando la cámara...", "カメラを起動中...", "正在启动相机..."};
inline constexpr Text kCameraFailed{
    "Camera not available. Key 5 turns it off.", "Camera non disponibile. Il tasto 5 la spegne.",
    "Caméra indisponible. La touche 5 l'éteint.", "Kamera nicht verfügbar. Taste 5 schaltet sie aus.",
    "Cámara no disponible. La tecla 5 la apaga.", "カメラを使用できません。5 でオフ。",
    "相机不可用。按 5 关闭。"};
inline constexpr Text kAlignSteps{
    "1. Point at a bright star you know.\n"
    "2. Arrows: push the drawn star onto the real one.\n"
    "3. Star near an edge: 7/8 narrow or widen. 6: Done.",
    "1. Inquadra una stella luminosa che conosci.\n"
    "2. Frecce: porta la stella disegnata su quella vera.\n"
    "3. Stella vicino al bordo: 7/8 stringe o allarga. 6: fine.",
    "1. Visez une étoile brillante connue.\n"
    "2. Flèches : amenez l'étoile dessinée sur la vraie.\n"
    "3. Étoile près du bord : 7/8 resserre ou élargit. 6 : fin.",
    "1. Auf einen bekannten hellen Stern zielen.\n"
    "2. Pfeile: gezeichneten Stern auf den echten schieben.\n"
    "3. Stern am Rand: 7/8 enger oder weiter. 6: fertig.",
    "1. Apunta a una estrella brillante conocida.\n"
    "2. Flechas: lleva la estrella dibujada sobre la real.\n"
    "3. Estrella cerca del borde: 7/8 estrecha o amplía. 6: listo.",
    "1. よく知っている明るい星に向ける。\n2. 矢印：描いた星を本物に重ねる。\n"
    "3. 端の星：7/8 で画角調整。6：完了。",
    "1. 对准一颗熟悉的亮星。\n2. 方向键：把绘制的星移到真实星上。\n"
    "3. 星在边缘：7/8 缩小或放大视场。6：完成。"};

// About page.
inline constexpr Text kAboutTitle{"About", "Informazioni", "À propos", "Über", "Acerca de", "情報", "关于"};
inline constexpr Text kAboutBody{
    "A pocket planetarium for the M5Stack CardputerZero.\n## Manual\n"
    "Scan the QR code with your phone for the full manual.\n## Developer\n"
    "Andrea Salvatore (IU4APC)\n## License\nMIT. The sky data keep their own licenses: see Credits.",
    "Un planetario tascabile per M5Stack CardputerZero.\n## Manuale\n"
    "Inquadra il QR code col telefono per il manuale completo.\n## Sviluppatore\n"
    "Andrea Salvatore (IU4APC)\n## Licenza\nMIT. I dati del cielo hanno le loro licenze: vedi Crediti.",
    "Un planétarium de poche pour le M5Stack CardputerZero.\n## Manuel\n"
    "Scannez le QR code avec votre téléphone pour le manuel complet.\n## Développeur\n"
    "Andrea Salvatore (IU4APC)\n## Licence\nMIT. Les données du ciel ont leurs propres licences : voir Crédits.",
    "Ein Taschenplanetarium für den M5Stack CardputerZero.\n## Handbuch\n"
    "QR-Code mit dem Handy scannen für das vollständige Handbuch.\n## Entwickler\n"
    "Andrea Salvatore (IU4APC)\n## Lizenz\nMIT. Die Himmelsdaten haben eigene Lizenzen: siehe Credits.",
    "Un planetario de bolsillo para el M5Stack CardputerZero.\n## Manual\n"
    "Escanea el código QR con el móvil para el manual completo.\n## Desarrollador\n"
    "Andrea Salvatore (IU4APC)\n## Licencia\nMIT. Los datos del cielo tienen sus propias licencias: ver Créditos.",
    "M5Stack CardputerZero のポケットプラネタリウム。\n## マニュアル\n"
    "QRコードをスマートフォンで読み取ると完全なマニュアルが開きます。\n## 開発者\n"
    "Andrea Salvatore (IU4APC)\n## ライセンス\nMIT。星のデータは個別のライセンスです：クレジット参照。",
    "M5Stack CardputerZero 的口袋天象仪。\n## 手册\n"
    "用手机扫描二维码查看完整手册。\n## 开发者\n"
    "Andrea Salvatore (IU4APC)\n## 许可\nMIT。星空数据有各自的许可：见致谢。"};
inline constexpr Text kCredits{"Credits", "Crediti", "Crédits", "Credits", "Créditos", "クレジット", "致谢"};

} // namespace zerosky::text
