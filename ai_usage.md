Functiile cerute

Am folosit Gemini exclusiv pt parse\_condition si match\_condition de la comanda filter.



Ce i-am scris

I-am dat tot fisierul de dinainte ca context ca sa stie structura si variabilele si i-am cerut sa faca parse\_condition care sa ia un string de genul field:operator:value si sa il sparga in 3. Dupa i-am zis sa faca match\_condition care sa verifice daca raportul se potriveste la category si severity.



Ce a generat AI-ul

Mi-a dat un cod destul de basic care mergea cu strchr sa gaseasca : si sa puna \\0 acolo. La match\_condition a facut pur si simplu niste if-uri obisnuite folosind strcmp si atoi.



Ce am mofidicat eu si de ce

Logica lui strica stringul original din argv pentru ca scria direct peste el. Ca sa nu dea crash am bagat eu in functie un buffer char temp\[256] si am dat cu strncpy ca sa lucrez in siguranta pe o copie a stringului, fara sa stric parametrii.



Ce am invatat din asta

Am inteles cum sta treaba cu pointerii, mai exact la manipularea de stringuri in C. E mult mai eficient sa pui un \\0 direct unde ai nevoie ca sa rupi un cuvant in loc sa te complici mereu cu functii gen strtok care nu sunt thread safe.

