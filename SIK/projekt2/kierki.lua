-- Celem zadania jest zaimplementowanie serwera i klienta gry w kierki. Serwer przeprowadza grę. Klienty reprezentują graczy.

-- ### Zasady gry

-- W kierki gra czterech graczy standardową 52-kartową talią. Gracze siedzą przy stole na miejscach N (ang. _north_), E (ang. _east_), S (ang _south_), W (ang. _west_). Rozgrywka składa się z rozdań. W każdym rozdaniu każdy z graczy otrzymuje na początku po 13 kart. Gracz zna tylko swoje karty. Gra składa się z 13 lew. W pierwszej lewie wybrany gracz wychodzi, czyli rozpoczyna rozdanie, kładąc wybraną swoją kartę na stół. Po czym pozostali gracze w kolejności ruchu wskazówek zegara dokładają po jednej swojej karcie. Istnieje obowiązek dokładania kart do koloru. Jeśli gracz nie ma karty w wymaganym kolorze, może położyć kartę w dowolnym innym kolorze. Nie ma obowiązku przebijania kartą starszą. Gracz, który wyłożył najstarszą kartę w kolorze karty położonej przez gracza wychodzącego, bierze lewę i wychodzi jako pierwszy w następnej lewie. Obowiązuje standardowe starszeństwo kart (od najsłabszej): 2, 3, 4, …, 9, 10, walet, dama, król, as.

-- W grze chodzi o to, żeby brać jak najmniej kart. Za branie kart otrzymuje się punkty. Wygrywa gracz, który w całej rozgrywce zbierze najmniej punktów. Jest siedem typów rozdań:

-- 1.  nie brać lew, za każdą wziętą lewę dostaje się 1 punkt;
-- 2.  nie brać kierów, za każdego wziętego kiera dostaje się 1 punkt;
-- 3.  nie brać dam, za każdą wziętą damę dostaje się 5 punktów;
-- 4.  nie brać panów (waletów i króli), za każdego wziętego pana dostaje się 2 punkty;
-- 5.  nie brać króla kier, za jego wzięcie dostaje się 18 punktów;
-- 6.  nie brać siódmej i ostatniej lewy, za wzięcie każdej z tych lew dostaje się po 10 punktów;
-- 7.  rozbójnik, punkty dostaje się za wszystko wymienione powyżej.

-- Rozdania nie trzeba rozgrywać do końca – można je przerwać, jeśli już wiadomo, że wszystkie punkty zostały rozdysponowane.

-- ### Parametry wywołania serwera

-- Parametry wywołania serwera mogą być podawane w dowolnej kolejności. Jeśli parametr został podany więcej niż raz, to obowiązuje jego pierwsze lub ostatnie wystąpienie na liście parametrów.

--     -p <port>
    

-- Określa numer portu, na którym ma nasłuchiwać serwer. Parametr jest opcjonalny. Jeśli nie został podany lub ma wartość zero, to wybór numeru portu należy scedować na wywołanie funkcji `bind`.

--     -f <file>
    

-- Określa nazwę pliku zawierającego definicję rozgrywki. Parametr jest obowiązkowy.

--     -t <timeout>
    

-- Określa maksymalny czas w sekundach oczekiwania serwera. Jest to liczba dodatnia. Parametr jest opcjonalny. Jeśli nie podano tego parametru, czas ten wynosi 5 sekund.

-- ### Parametry wywołania klienta

-- Parametry wywołania klienta mogą być podawane w dowolnej kolejności. Jeśli parametr został podany więcej niż raz lub podano sprzeczne parametry, to obowiązuje pierwsze lub ostatnie wystąpienie takiego parametru na liście parametrów.

--     -h <host>
    

-- Określa adres IP lub nazwę hosta serwera. Parametr jest obowiązkowy.

--     -p <port>
    

-- Określa numer portu, na którym nasłuchuje serwer. Parametr jest obowiązkowy.

--     -4
    

-- Wymusza w komunikacji z serwerem użycie IP w wersji 4. Parametr jest opcjonalny.

--     -6
    

-- Wymusza w komunikacji z serwerem użycie IP w wersji 6. Parametr jest opcjonalny.

-- Jeśli nie podano ani parametru `-4`, ani `-6`, to wybór wersji protokołu IP należy scedować na wywołanie funkcji `getaddrinfo`, podając `ai_family = AF_UNSPEC`.

--     -N
--     -E
--     -S
--     -W
    

-- Określa miejsce, które klient chce zająć przy stole. Parametr jest obowiązkowy.

--     -a
    

-- Parametr jest opcjonalny. Jeśli jest podany, to klient jest automatycznym graczem. Jeśli nie jest podany, to klient jest pośrednikiem między serwerem a graczem-użytkownikiem.

-- ### Protokół komunikacyjny

-- Serwer i klient komunikują się za pomocą TCP. Komunikaty są napisami ASCII zakończonymi sekwencją `\r\n`. Oprócz tej sekwencji w komunikatach nie ma innych białych znaków. Komunikaty nie zawierają terminalnego zera. Miejsce przy stole koduje się literą `N`, `E`, `S` lub `W`. Typ rozdania koduje się cyfrą od `1` do `7`. Numer lewy koduje się liczbą od `1` do `13` zapisaną przy podstawie 10 bez zer wiodących. Przy kodowaniu kart najpierw podaje się wartość karty:

-- *   `2`, `3`, `4`, …, `9`, `10`, `J`, `Q`, `K`, `A`.

-- Następnie podaje się kolor karty:

-- *   `C` – ♣, trefl, żołądź (ang. _club_),
-- *   `D` – ♦, karo, dzwonek (ang. _diamond_),
-- *   `H` – ♥, kier, czerwień (ang. _heart_),
-- *   `S` – ♠, pik, wino (ang. _spade_).

-- Serwer i klient przesyłają następujące komunikaty.

--     IAM<miejsce przy stole>\r\n
    

-- Komunikat wysyłany przez klienta do serwera po nawiązaniu połączenia. Informuje, które miejsce przy stole chce zająć klient. Jeśli klient nie przyśle takiego komunikatu w czasie `timeout`, serwer zamyka połączenie z tym klientem. W ten sposób serwer traktuje również klienta, który po nawiązaniu połączenia przysłał błędny komunikat.

--     BUSY<lista zajętych miejsc przy stole>\r\n
    

-- Komunikat wysyłany przez serwer do klienta, jeśli wybrane miejsce przy stole jest już zajęte. Jednocześnie informuje go, które miejsca przy stole są zajęte. Po wysłaniu tego komunikatu serwer zamyka połączenie z klientem. W ten sposób serwer traktuje również klienta, który próbuje podłączyć się do trwającej rozgrywki.

--     DEAL<typ rozdania><miejsce przy stole klienta wychodzącego jako pierwszy w rozdaniu><lista kart>\r\n
    

-- Komunikat wysyłany przez serwer do klientów po zebraniu się czterech klientów. Informuje o rozpoczęciu rozdania. Lista zawiera 13 kart, które klient dostaje w tym rozdaniu.

--     TRICK<numer lewy><lista kart>\r\n
    

-- Komunikat wysyłany przez serwer do klienta z prośbą o położenie karty na stole. Lista kart zawiera od zera do trzech kart aktualnie leżących na stole. Jeśli klient nie odpowie w czasie `timeout`, to serwer ponawia prośbę. Komunikat wysyłany przez klienta do serwera z kartą, którą klient kładzie na stole (lista kart zawiera wtedy jedną kartę).

--     WRONG<numer lewy>\r\n
    

-- Komunikat wysyłany przez serwer do klienta, który przysłał błędny komunikat w odpowiedzi na komunikat `TRICK`. Wysyłany również wtedy, gdy klient próbuje położyć kartę na stole nieproszony o to. Zawartość błędnego komunikatu jest ignorowana przez serwer.

--     TAKEN<numer lewy><lista kart><miejsce przy stole klienta biorącego lewę>\r\n
    

-- Komunikat wysyłany przez serwer do klientów. Informuje, który z klientów wziął lewę. Lista kart zawiera cztery karty składające się na lewę w kolejności, w jakiej zostały położone na stole.

--     SCORE<miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów>\r\n
    

-- Komunikat wysyłany przez serwer do klientów po zakończeniu rozdania. Informuje o punktacji w tym rozdaniu.

--     TOTAL<miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów><miejsce przy stole klienta><liczba punktów>\r\n
    

-- Komunikat wysyłany przez serwer do klientów po zakończeniu rozdania. Informuje o łącznej punktacji w rozgrywce.

-- Klient ignoruje błędne komunikaty od serwera.

-- Po zakończeniu rozgrywki serwer rozłącza wszystkie klienty i kończy działanie. Po rozłączeniu się serwera klient kończy działanie.

-- Jeśli klient rozłączy się w trakcie rozgrywki, to serwer zawiesza rozgrywkę w oczekiwaniu na podłączenie się klienta na puste miejsce przy stole. Po podłączeniu się klienta serwer przekazuje mu stan aktualnego rozdania. Za pomocą komunikatu `DEAL` przekazuje karty, które klient dostał w tym rozdaniu. Za pomocą komunikatów `TAKEN` przekazuje dotychczas rozegrane lewy. Następnie serwer wznawia rozgrywkę i wymianę komunikatów `TRICK`.

-- Wireshark dissector
kierki_proto = Proto("kierki", "Kierki Protocol")
local f_type = ProtoField.string("kierki.type", "Packet Type", FT_STRING)

-- common
local f_cards = ProtoField.string("kierki.cards", "Cards", FT_STRING)
local f_number = ProtoField.string("kierki.number", "Trick Number", FT_STRING)

-- IAM
local f_iam = ProtoField.string("kierki.iam.place", "Place", FT_STRING)

-- BUSY
local f_busy = ProtoField.string("kierki.taken.places", "Places", FT_STRING)

-- DEAL
local f_deal_type = ProtoField.string("kierki.deal.type", "Deal Type", FT_STRING)
local f_deal_first = ProtoField.string("kierki.deal.first", "First", FT_STRING)
-- f_cards

-- TRICK
-- f_cards
-- f_number

-- WRONG
-- f_number

-- TAKEN
-- f_number
-- f_cards
local f_taken_place = ProtoField.string("kierki.taken.place", "Place", FT_STRING)

-- SCORE
local f_score_1 = ProtoField.string("kierki.score.1", "Points 1", FT_STRING)
local f_dir_1 = ProtoField.string("kierki.dir.1", "Direction 1", FT_STRING)
local f_score_2 = ProtoField.string("kierki.score.2", "Points 2", FT_STRING)
local f_dir_2 = ProtoField.string("kierki.dir.2", "Direction 2", FT_STRING)
local f_score_3 = ProtoField.string("kierki.score.3", "Points 3", FT_STRING)
local f_dir_3 = ProtoField.string("kierki.dir.3", "Direction 3", FT_STRING)
local f_score_4 = ProtoField.string("kierki.score.4", "Points 4", FT_STRING)
local f_dir_4 = ProtoField.string("kierki.dir.4", "Direction 4", FT_STRING)

-- TOTAL
-- f_score_1
-- f_score_2
-- f_score_3
-- f_score_4

kierki_proto.fields = {f_type, f_cards, f_number, f_iam, f_busy, f_deal_type, f_deal_first, f_taken_place, f_score_1, f_score_2, f_score_3, f_score_4}

local possible_types = {
    "IAM",
    "BUSY",
    "DEAL",
    "TRICK",
    "WRONG",
    "TAKEN",
    "SCORE",
    "TOTAL"
}

function kierki_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "Kierki"
    local subtree = tree:add(kierki_proto, buffer(), "Kierki Protocol Data")
    local str = buffer():string()

    -- remove \r\n
    if str:sub(#str - 1) == "\r\n" then
        str = str:sub(1, #str - 2)
    else
        subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "No \\r\\n at the end")
    end

    -- looking for type
    local found = ""
    for i, type in ipairs(possible_types) do
        if str:sub(1, #type) == type then
            subtree:add(f_type, buffer(0, #type))
            str = str:sub(#type + 1)
            found = type
            break
        end
    end

    if found == "" then
        subtree:add(f_type, "UNKNOWN")
        subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "Unknown type")
        return
    end

    if str == "" then
        return
    end

    if found == "IAM" then
        subtree:add(f_iam, buffer(#found, #str))
    elseif found == "BUSY" then
        subtree:add(f_busy, buffer(#found, #str))
    elseif found == "DEAL" then
        subtree:add(f_deal_type, buffer(#found, 1))
        subtree:add(f_deal_first, buffer(#found + 1, 1))
        subtree:add(f_cards, buffer(#found + 2, #str))
    elseif found == "TRICK" then
        subtree:add(f_number, buffer(#found, 1))
        subtree:add(f_cards, buffer(#found + 1, #str))
    elseif found == "WRONG" then
        subtree:add(f_number, buffer(#found, #str))
    elseif found == "TAKEN" then
        local taken_place = str:sub(#str - 1)
        str = str:sub(1, #str - 2)
        local number = str:sub(1, 1)
        local cards = str:sub(2)
        subtree:add(f_number, buffer(#found, 1))
        subtree:add(f_cards, buffer(#found + 1, #str - 1))
        subtree:add(f_taken_place, buffer(#str, 1))
    elseif found == "SCORE" then
        local offset = 0
        subtree:add(f_dir_1, buffer(#found, 1))
        offset = offset + 1
        local score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_1, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_2, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_2, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_3, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_3, buffer(#found + offset, #score))
        offset = offset + #score
        subtree:add(f_dir_4, buffer(#found + offset, 1))
        offset = offset + 1
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_4, buffer(#found + offset, #score))
    elseif found == "TOTAL" then
        local offset = 0
        local score = str.gmatch(str, "%d+")
        subtree:add(f_score_1, buffer(#found, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_2, buffer(#found + offset, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_3, buffer(#found + offset, #score))
        offset = offset + #score
        score = str.gmatch(str:sub(offset), "%d+")
        subtree:add(f_score_4, buffer(#found + offset, #score))
    end
end
        
-- register our protocol
local tcp_port = DissectorTable.get("tcp.port")
for i = 21370, 21375 do
    tcp_port:add(i, kierki_proto)
end
-- add heuristic dissector if tcp stream starts with IAM, its probably kierki
kierki_proto:register_heuristic("tcp", function(buffer, pinfo, tree)
    local str = buffer():string()
    if str:sub(1, 3) == "IAM" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 4) == "BUSY" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 4) == "DEAL" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TRICK" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "WRONG" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TAKEN" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "SCORE" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
    if str:sub(1, 5) == "TOTAL" then
        kierki_proto.dissector(buffer, pinfo, tree)
    end
end)