#include <check.h>

#include "http.h"


START_TEST(check_method_test)
{
    ck_assert_int_eq(check_method("PUT"), 1);
    ck_assert_int_eq(check_method("GET"), 1);
    ck_assert_int_eq(check_method("HEAD"), 1);
    ck_assert_int_eq(check_method("DELETE"), 0);
    ck_assert_int_eq(check_method("POST"), 0);
}
END_TEST

START_TEST(check_http_version_test)
{
    ck_assert_int_eq(check_http_version("1.1"), 1);
    ck_assert_int_eq(check_http_version("1.0"), 0);
}
END_TEST

START_TEST(check_filename_test)
{
    ck_assert_int_eq(check_filename("hellowrold/"), 0);
    ck_assert_int_eq(check_filename(".hellowrold"), 0);
    ck_assert_int_eq(check_filename("hellowrold"), 1);
    ck_assert_int_eq(check_filename("asdfASDF09123-_"), 1);
    ck_assert_int_eq(check_filename(".}"), 0);
}
END_TEST

START_TEST(validate_test)
{
    struct httpRequest correct, bad_file, bad_method, bad_version;


    strcpy(correct.filename, "asdfndfjQQQQ");
    strcpy(correct.method, "GET");
    strcpy(correct.httpversion, "1.1");
    
    strcpy(bad_file.filename, ".}");
    strcpy(bad_file.method, "GET");
    strcpy(bad_file.httpversion, "1.1");
    
    strcpy(bad_method.filename, "file");
    strcpy(bad_method.method, "DELETE");
    strcpy(bad_method.httpversion, "1.1");
    
    strcpy(bad_version.filename, "asdfndfjQQQQ");
    strcpy(bad_version.method, "GET");
    strcpy(bad_version.httpversion, "1.0");

    ck_assert_int_eq(validate(correct), 1);
    ck_assert_int_eq(validate(bad_file), 0);
    ck_assert_int_eq(validate(bad_method), 0);
    ck_assert_int_eq(validate(bad_version), 0);
}
END_TEST

Suite * testing_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("testing");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, check_method_test);
    tcase_add_test(tc_core, check_http_version_test);
    tcase_add_test(tc_core, check_filename_test);
    tcase_add_test(tc_core, validate_test);
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

