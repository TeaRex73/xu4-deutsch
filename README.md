# ULTIMA IV: DIE QUESTE DES AVATARS auf Deutsch

Diesem klassischen Werke zu Ehren jetzt erstmals komplett auf deutsch - inklusive der Papierdokumente und Stoffkarte des Originals als PDFs!

**Original © 1985-1987 Lord British und Origin Systems, Inc. Alle Rechte vorbehalten**  
**Modernes Remake © 2002-2020 Das XU4 Team, veröffentlicht als Freie Software unter GPLv2**  
**Deutsche Fassung © 2013-2024 Finire Dragon UDIC, ebenfalls unter GPLv2**

Die Lizenz befindet sich in der Datei COPYING.

## Was ist das?

Origin Systems, Inc. publizierte im Jahre 1985 das Computer-Fantasy-Rollenspiel "Ultima IV", das als Meilenstein des Genres gilt,
da es als eines der ersten solchen Spiele eine Geschichte erzählte, die weit von "Töte den Oberbösewicht und gewinne somit das Spiel!" entfernt war.

Das Spiel erschien damals nur in englischer Sprache, auch erfolgreiche Computerspiele wurden üblicherweise noch nicht in andere Sprachen übersetzt. Daher war es bisher nie auf Deutsch erhältlich. Bis jetzt!

## Was bekomme ich hier?

Der hier erhältliche Code basiert auf dem bereits 2002 erstmals veröffentlichten Projekt "xu4", wurde aber komplett aus dem Englischen ins Deutsche übersetzt und verwendet Grafik und Sound aus der Apple-II-Originalversion von "Ultima IV"
statt wie xu4 aus der später entstandenen MS-DOS-Version. Sound und Musik sind dabei von einem echten Apple II mit Mockinboard-Soundkarte aufgenommen worden.

Die Originaldateien aus der MS-DOS-Version sind aus urheberrechtlichen Gründen nicht enthalten; sie dürfen aber von bestimmten anderen Personen legal im Internet zum Download angeboten werden und werden beim ersten Start automatisch heruntergeladen.

Dies ist Version U4DEU-2025-02-06 "We just found out we could release!" für Microsoft Windows

## Systemvoraussetzungen

Windows 7 oder höher, 32 oder 64 Bit Intel-kompatibler Prozessor, Pentium IV oder höher.

## Warnung

Achtung: Diese Version ist noch eine Vorab-Version, also wie man sagt, UNVOLLSTÄN UND FÄHLERHAFT, insbesondere hat sie noch keine Tests durch andere Personen als mich selbst durchlaufen! Auch fehlen Tests auf Windows 11.
Fehler bitte unbedingt melden! Pull requests sind natürlich ebenfalls willkommen!

## Wie benutze ich es?

Diese Version ist für Windows. Es gibt keinen Installer, sonder es ist portable Software nach dem Prinzip "Nur entpacken und starten". Bitte die heruntergeladene ZIP-Datei
einfach irgendwohin entpacken und dann den Link "Ultima IV" aus dem Hauptordner
starten. Das Icon für den Link sieht nur dann richtig aus, wenn sich der
Hauptordner auf demDesktop des aktuellen Benutzers befindet (eine durch Windows bedingte technische Einschränkung für portable Programme ohne Installer), der Link funktioniert aber
auch sonst.

Beim ersten Doppelklick auf den Link "Ultima IV" im Hauptordner wird nun die
vom Spiel benötigte englischsprachige Originalversion von Ultima IV
für MS-DOS einmalig aus dem Internet heruntergeladen. Ein zweiter Klick auf
den gleichen Link wandelt die enthaltene Dokumentation in normale, lesbare PDF-Dateien um.
Ein dritter Klick (und jeder spätere) startet dann das Spiel.

## Das Spiel gefällt mir gar nicht, ich will es nicht mehr auf meinem PC haben!

Zum "Deinstallieren" einfach den kompletten Ordner löschen. Wenn nötig, kann
noch der Ordner "%AppData%\xu4" entfernt werden, dort findet man den
Spielstand und eine kleine Konfigurationsdatei, diese Dateien belegen
aber insgesamt weniger als 4 Kilobytes und stören nicht weiter.

## Mehr Details

Diese Version beruht auf der Raspberry-Pi-Version und wurde mit MinGW (MSYS2) für Windows übersetzt - es gibt 32-Bit
und 64-Bit-Versionen, die 32-Bit-Version läuft aber ohne weiteres auch auf 64-Bit Windows. Ansonsten gilt die Beschreibung
der Raspberry-Version in "RASPBERR.TXT" auch hierfür. Die Dokumentation ist noch nicht auf Windows
angepasst, sollte aber wohl kein Problem sein.

Behobene Bugs seit der vorigen Version:
- Auch die Dokumentation enthält nun keine nutzbaren urheberrechtlich geschützten Inhalte von Origin Systems, Inc. oder Electronic Arts mehr. Erst mit Hilfe der Originaldatei werden diese erzeugt. Damit dürfte nun alles saubere Open Source sein.
- Kleine Fehlerbehebungen

Alle Bugs dürfen sehr gerne an mich gemeldet werden, einfach ein Issue hier eröffnen.
Auch sonstiges Feedback ist willkommen!

## To Do
- Workflows hinzufügen, die für ein Tag oder Release den Tag statt des Git-Hashs in den Dateinamen verwenden
- Karma-Punktevergabe an die Apple II Version angleichen, wo noch Abweichungen sind
- Abweichungen zu xu4 und zu den Originalversionen (Apple II und MS-DOS) vollständig dokumentieren
- Abweichungen zu den Originalversionen abschaltbar machen, soweit technisch machbar
- Changelog mit den einzelnen Änderungen seit 2013 erstellen (momentan nur im Git-Log zu lesen)
- Beta-Tester gewinnen und Tests durchführen lassen (das bedeutet Dich!)
- Raspberry-Pi-Version (bisher händisch außerhalb von Github gepflegt) automatisiert hier bauen lassen
- Raspberry-Pi-Version von der veralteten DispmanX-API auf KMS oder eine andere moderne API umziehen, so dass sie auch auf Pi's nach dem Pi 3 und auf neuen Pi-OS-Versionen noch läuft
- Von SDL 1.2.15 auf SDL 2 oder SDL 3 portieren
- Enhancement-Idee: In Kämpfen die Himmelsrichtung berücksichtigen, aus der der Feind kam, so dass die Gegner nicht immer im Norden und die Spielercharaktere nicht immer im Süden stehen.

Wenn Du Dich hier angesprochen fühlst, ist jede Mitarbeit willkommen!

## Danksagungen

VIELEN DANK an Richard Garriott, Origin System Inc. und an das xu4-Team, ohne
die nichts davon möglich gewesen wäre! An Sean Gugler für u4remasteredA2 und
an ergonomy_joe für u4-decompiled (beide auf GitHub zu finden), welche mir
sehr halfen, die Original-Algorithmen detailliert zu verstehen.
An meine liebe unvergessene Schwester Ebba Zahner(†) und meinen Schwager Jens
Zahner, die mir 2013 den ersten Raspberry Pi zu Weihnachten geschenkt und mich
damit zu diesem Projekt inspiriert haben. Und natürlich vor allen anderen an
meine Frau Anja Ticmanis und meine Kinder,
die meinen großen Zeiteinsatz für dieses Projekt immer ertragen und
unterstützt haben! Ihnen widme ich dieses Projekt!

## Schlusswort

In diesem Projekt stecken viele Stunden meiner Freizeit der letzten zwölf
Jahre, die aber hoffentlich gut investiert waren. Viel Spaß damit (den ich
auch beim Erstellen hatte) wünscht Euch Euer

Finire Dragon UDIC (bürgerlich Linards Ticmanis, TeaRex73 auf GitHub)
