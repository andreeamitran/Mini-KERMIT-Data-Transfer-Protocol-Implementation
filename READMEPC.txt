UNIVERSITATEA POLITEHNICA BUCURESTI
Facultatea de Automatica si Calculatoare, CTI
Student: Andreea Mitran
Grupa: 324CB
 _______    _______    _______    ______     _______    _______
(  ____ )  (  ____ \  (  ___  )  (  __  \   (       )  (  ____ \
| (    )|  | (    \/  | (   ) |  | (  \  )  | () () |  | (    \/
| (____)|  | (__      | (___) |  | |   ) |  | || || |  | (__    
|     __)  |  __)     |  ___  |  | |   | |  | |(_)| |  |  __)   
| (\ (     | (        | (   ) |  | |   ) |  | |   | |  | (      
| ) \ \__  | (____/\  | )   ( |  | (__/  )  | )   ( |  | (____/\
|/   \__/  (_______/  |/     \|  (______/   |/     \|  (_______/


=========================		TEMA1 PC		=============================

	Salut!
	
	Voi explica ceea ce am facut in tema, bazandu-ma pe cele doua fisiere
in care am scris implementarea, ksender si kreceiver.

===========================		ksender.c		=============================

	Am utilizat o functie, ce are antetul urmator:
	msg pkg (unsigned char len, char type, char *data, unsigned char seq),
in care realizez formarea unui packet ce trebuie trimis catre receiver.
In functie de parametri primiti si de valorile date in cerinta, completez
campurile din structura unui pachet mini kermit in felul urmator:
	-> campurile soh si mark sunt completate conform enuntului
	-> campurile type, seq, len si data sunt completate cu informatii-
		le primite ca si parametri
	-> calculez crc-ul, copiind intr-un buffer de tip char* campurile
		completate pana in acest moment si aplicand functia data in
		schelet, crc16_ccitt
	-> crc-ul este pus in campul check, iar valoarea campului mark este
		cea data in enunt.
	Pentru a scrie in payload, ma folosesc de buffer-ul deja completat cu
campurile pana la check. Deoarece acesta este declarat unsigned short, voi
lua primii 8 biti ultimii 8 biti, impartindu-l astfel in 2 bytes, pentru a
putea fi plasati in buffer. In final, se completeaza si ultimul byte din
buffer prin punerea campului mark din structura.
	Declar o variabila de tip msg si copiez buffer-ul in payload, iar lun-
gimea acestuia va fi cea data de campul len + 2.

	In functia send se realizeaza operatia de trimitere de pachete, care
poate trimite orice tip de pachet, intrucat primeste ca si parametru o
variabila de tip msg si numele fisierului (pentru a facilita afisarea me-
sajelor).
	Salvez valoarea acestuia intr-o variabila send, pe care o voi trimite
intr-un while, in felul urmator:
	-> se trimite mesajul
	-> se asteapta raspuns, folosind functia receive_message_timeout(5000).
	-> daca a avut loc timeout si nu a primit ACK/NAK, se retrimite
	pachetul anterior, pentru a primi un nou raspuns
	-> daca se realizeaza timeout si a 3-a oara consecutiv, atunci se in-
	trerupe programul si se iese prin functia exit
	-> altfel, daca crc-ul calculat pe pachet este diferit de check-ul
	primit din pachet, retrimite pachet curent si asteapta alt raspuns
	-> altfel, daca seq pachetului primit este diferit de seq pachetului
	trimis, atunci inseamna ca nu s-a primit ACK/NAK pentru pachetul ante-
	rior si se retrimite, pentru a primi un nou raspuns
	-> altfel, daca datele sunt corupte si se primeste un raspuns de tipul
	NAK, se retrimite ultimul pachet si se asteapta un nou raspuns
	-> altfel, daca pachetul trimis a fost de tipul B, inseamna ca este
	sfarsitul transmisiei si se va iesi din bucla while
	-> altfel, daca operatia de trimitere a pachetului si primirea raspun-
	sului este este realizata cu succes, se iese din bucla while
	Functia aceasta este folosita in main, pentru care voi explica mai de-
parte. M-a ajutat foarte mult, intrucat am evitat folosirea de cod duplicat
pentru fiecare pachet separat si am incercat sa evit pe cat posibil sa hard-
codez.

	In functia main, realizez construirea campului data din pachetul
send_init, completand conform cerintei fiecare dintre "campurile" sale.
Spun "campurile", intrucat nu am folosit o alta structura pentru aceste
informatii, ci mi s-a parut mai usor sa iau un vector de tip char*, iar la
fiecare index se afla un nou "camp" din data:
	-> pe pozitia 0 se afla MAXL, pe pozitia 1 se afla TIME, etc.
	Printr-o bucla ce se realizeaza pentru fiecare argument primit in linia
de comanda, realizez trimiterea pachetelor file_header, data si EOF. Dupa
deschiderea fisierului si trimiterea pachetului file_header, cat timp nu
s-a ajuns la EOF, citesc cate MAXL bytes din fisier si ii salvez in datas,
iar folosind functia send, trimit pachetele de tip data.
	In final, trimit pachetul de tip B, end of transmission.
	
=============================================================================

===========================		kreveiver.c		=============================

	In fisierul kreceiver.c, am implementat in primul rand o functie ce are
antetul in felul urmator:
	msg ack_nak_pkg (char seq, char type).
	Aceasta realizeaza construirea unui pachet avand tipul ACK/NAK, in
functie de valoarea parametrului type. Construirea acestuia se realizeaza in
mod asemanator cu modul in care se construiesc pachetele in functia din fi-
sierul ksender.c si este explicata mai sus.

	Am o functie ce construieste numele header-ului in functie de numele
primit prin pachetul file_header in ksender.

	In functia main, utilizand un while, astept initial un pachet de la
sender. Daca acesta depaseste de 3 ori cele 5 secunde, atunci se va intre-
rupe transmisia, altfel, este ceruta retrimiterea pachetului.
	Apoi, daca difera crc-ul calculat pe pachet in kreceiver de check-ul
primit in pachet de la sender, atunci se va trimite un pachet NAK si se va
astepta retrimiterea pachetului catre receiver.
	Daca secventa pachetului trimis este aceeasi cu secventa pachetului
primit cu succes anterior, atunci inseamna ca nu este in regula si ca pa-
chetu la fost deja primit cu succes si va cere trimiterea unui nou pachet.
	Apoi, daca pachetul trimis cu succes este de tipul F, se va crea nume-
le unui nou fisier, folosit numele trimis de pachet si adaugarea unui "recv"
in fata si se va deschide fisierul. Daca pachetul este de tip data, atunci
se va scrie in fisier ceea ce s-a primit. Daca pachetul este de tipul Z,
atunci se va inchide fisierul in care s-a scris, iar daca pachetul este de
tipul B, se realizeaza terminarea buclei while, se trimite ACK si se incheie
transmisia.
	Retin de fiecare data seq pachetului primit cu succes la pasul curent,
astfel incat la urmatoarea trimitere de pachet, in cazul in care se intampla
timeout si va fi trimis de prea multe ori un pachet, acesta sa fie "dropped"
si sa se astepte un urmator pachet.

=============================================================================

	In fisierul lib.h am declarat structura unui pachet mini_kermit si o
functie care afiseaza mesaj de eroare (pentru cazul in care nu pot citi/
scrie in fisier) si intrerupe programul.

	Am o functie care calculeaza (defapt formeaza crc-ul) primit din
payload-ul pachetului. Intrucat acesta nu incape intr-un byte, ma folosesc
de antepenultimul si penultimul byte in payload, pentru a "reface" crc-ul.

	Acestea fiind zise, aceasta este descrierea modului in care am imple-
mentat aceasta tema, unele detalii apar si in comentariile din cod. (Am uti-
lizat comentarii in engleza in cod, deoarece imi era mult mai usor de urma-
rit, in general variabilele si denumirile functiilor cu care lucrez sunt
in engleza si e totul mai lizibil).

	Numai bine,
	Andreea
