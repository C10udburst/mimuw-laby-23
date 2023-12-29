./mimpirun 7 valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes examples_build/bad_rank 2>&1 | grep "ERROR SUMMARY" | sed "s/[^ ]* //"
=====================================================================
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
