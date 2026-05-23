# AI usage - all phases

## Phase 1

### Functiile cerute
Am folosit gemini exclusiv pt parse_condition si match_condition de la comanda filter.

### Ce i-am scris
I-am dat tot fisierul de dinainte ca context ca sa stie structura si variabilele si i-am cerut sa faca parse_condition care sa ia un string de genul field:operator:value si sa il sparga in 3. Dupa i-am zis sa faca match_condition care sa verifice daca raportul se potriveste la category si severity

### Ce a generat AI-ul
Mi-a dat un cod destul de basic care mergea cu strchr sa gaseasca : si sa puna \0 acolo. La match_condition a facut pur si simplu niste if-uri obisnuite folosind strcmp si atoi

### Ce am modificat eu si de ce
Logica lui strica stringul original din argv pentru ca scria direct peste el. Ca sa nu dea crash am bagat eu in functie un buffer char temp[256] si am dat cu strncpy ca sa lucrez in siguranta pe o copie a stringului, fara sa stric parametrii

### Ce am invatat din asta
Am inteles cum sta treaba cu pointerii mai exact la manipularea de stringuri in c. E mult mai eficient sa pui un \0 direct unde ai nevoie ca sa rupi un cuvant in loc sa te complici mereu cu functii gen strtok care nu sunt thread safe

---

## Phase 2

### Functiile cerute
Am folosit AI-ul in faza 2 doar ca sa inteleg cum sa extind match_condition si ca sa ma ajute cu debugging-ul la procese si semnale cand nu imi mergea comunicarea cu monitoru in background.

### Ce i-am scris
I-am pus erorile pe care le primeam cand foloseam signal() normal si murea procesul cand dadeam add si l-am intrebat care e faza. L-am mai pus sa imi explice si cum sa fac cast la timestamp cu atol pt operatorii din filter ca nu imi iesea comparatia cu time_t deloc

### Ce a generat AI-ul
Mi-a explicat ca functia signal() e destul de nesigura si mi-a dat un exemplu teoretic despre cum se configureaza o structura de tip sigaction ca sa prind SIGUSR1. De asemenea mi-a explicat ca trebuie sa citesc pid-ul din fisier ca string si sa ii dau atoi ca sa pot da kill() din city_manager.

### Ce am modificat eu si de ce
Am integrat logica de sigaction scrisa de mine in monitor_reports. A trebuit sa modific putin si parsoarele de la filter ca AI-ul imi sugera niste chestii cu regex prea complicate si am vrut sa pastrez codul cat mai simplu pe if-uri.

### Ce am invatat din asta
Am vazut in practica de ce e mai ok sa folosim sigaction in loc de banalul signal si cum 2 procese diferite ajung sa comunice prin semnale doar pe baza unui fisier ascuns cu un pid pe disc. Am inteles si cum dai trigger la procese externe din c.

---

## Phase 3

### Functiile cerute
Am intrebat AI-ul ca sa ma ajute sa inteleg de ce imi ramaneau procesele blocate (zombie) si de ce imi crapa dup2() cand incercam sa leg outputu de la scorer la hub-ul principal.

### Ce i-am scris
I-am dat snippet-ul meu de cod cu pipe-urile din city_hub unde faceam fork si execl la scorer si l-am intrebat de ce textul nu ajunge pe ecran in hub si de ce se blocheaza terminalu de tot.

### Ce a generat AI-ul
AI-ul mi-a explicat ca pipe-urile se blocheaza infinit daca nu inchizi capetele nefolosite (gen capatu de citire in copil si capatu de scriere in parinte dupa ce trimiti). Mi-a sugerat si sa pun signal(SIGCHLD, SIG_IGN) la inceput in main ca sa nu mai generez procese zombie cand se termina alea mici.

### Ce am modificat eu si de ce
Am bagat inchiderile de pipe (close pe pipefd) exact inainte de waitpid in logica mea si a luat-o. Am ignorat niste chestii extra pe care mi le dadea el ca solutii pt un shell mai avansat pt ca depasea ce ne trebuie la proiect

### Ce am invatat din asta
A fost super util sa prind logica din spatele la dup2. Practic am pacalit procesu copil sa creada ca pipe-u este ecranul lui (stdout) normal si asa am reusit sa preiau textul lui direct in variabila din parinte fara sa folosesc fisiere txt de legatura