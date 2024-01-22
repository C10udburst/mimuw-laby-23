CREATE TABLE Mieszkaniec (
    ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    PESEL VARCHAR(11) NOT NULL UNIQUE, -- Zakładamy model jak w USOSie, czyli jak ktoś nie posiada PESELu to dostaje sztuczny PESEL
    Imie VARCHAR(25) NOT NULL,
    Nazwisko VARCHAR(25) NOT NULL, NOT NULL UNIQUE
    ID_Mieszkania INT NOT NULL,
    FOREIGN KEY (ID_Mieszkania) REFERENCES Mieszkanie(ID)
);

CREATE TABLE Mieszkanie (
    ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    Metraz INT NOT NULL,
    Adres VARCHAR(50) NOT NULL,
    ID_Wlasciciela INT NOT NULL,
    FOREIGN KEY (ID_Wlasciciela) REFERENCES Mieszkaniec(ID)
);