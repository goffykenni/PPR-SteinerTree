1. Prelozit jednoduse pomoci make (presunu se do tohoto adresare,
napisu make a enter) nebo "natvdro" prikazem:

mpiCC main.cpp checker.cpp unionfind.cpp stack.cpp graph.cpp -o steiner -lm

2. Spustit pomoci prikazu

./steiner <filename>

kde <filename> je nazev souboru se vstupnim grafem.

3. Format vstupniho souboru
    - Nejprve dve cela cisla, prvni udava pocet uzlu grafu, druhy pocet
    uzlu v terminalni mnozine.
    
    - Potom seznam uzlu terminalni mnoziny.

    - Potom matice incidence grafu. Graf musi byt souvisly. Nejlepsi je
    pouzit ty dve utility, co nam poskytl na cviceni a jsou na eduxu.
