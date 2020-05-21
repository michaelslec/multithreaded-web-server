#include <check.h>
#include <curl/curl.h>
#include "httpserver.h"

START_TEST(our_first_test)
{
    CURL *curl;
    CURLcode res;

}

END_TEST

Suite * testing_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("testing");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, our_first_test);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = testing_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
