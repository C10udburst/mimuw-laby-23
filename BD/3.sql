SELECT aut.id as autor, COALESCE(w.wp, 0) as wynik_publikacyjny
FROM autorzy aut
LEFT JOIN (SELECT agg.autor as autor, MAX(agg.sum) as wp FROM (
    SELECT ao.autor as autor, SUM(p.punkty/p.autorzy) OVER (
        PARTITION BY autor
        ORDER BY p.punkty
        ROWS BETWEEN CURRENT ROW AND 3 FOLLOWING
    ) as sum FROM autorstwo ao INNER JOIN prace p ON p.id = ao.praca
) agg GROUP BY autor) w
ON aut.id = w.autor ORDER BY autor;
