SELECT id as autor, erdos
FROM autorzy
LEFT JOIN (
    SELECT autor1, MIN(erdos) as erdos
    FROM (
        SELECT DISTINCT LEVEL - 1 as erdos, autor1
        FROM (
            SELECT DISTINCT a.autor as autor1, b.autor as autor2 FROM autorstwo a
            JOIN autorstwo b ON a.praca = b.praca
        )
        START WITH autor1 = 'Pilipczuk Mi'
        CONNECT BY NOCYCLE PRIOR autor1 = autor2
        AND LEVEL <= 8 -- sprawdzalem do 10, ale najwiekszy erdos to 6
    ) GROUP BY autor1
) ON id = autor1
ORDER BY erdos, autor;