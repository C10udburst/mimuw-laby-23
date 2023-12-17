CREATE INDEX e_src_idx ON e(src);
WITH
ee AS (SELECT tgt FROM e WHERE src=0),
f AS (SELECT e2.tgt, COUNT(*) AS cnt FROM e e2 JOIN ee ON e2.src=ee.tgt GROUP BY e2.tgt),
g AS (SELECT e3.tgt, SUM(f.cnt) AS cnt FROM e e3 JOIN f ON e3.src=f.tgt GROUP BY e3.tgt),
h AS (SELECT e4.tgt, SUM(g.cnt) AS cnt FROM e e4 JOIN g ON e4.src=g.tgt GROUP BY e4.tgt),
i AS (SELECT e5.tgt, SUM(h.cnt) AS cnt FROM e e5 JOIN h ON e5.src=h.tgt GROUP BY e5.tgt)
SELECT SUM(cnt) FROM i;
