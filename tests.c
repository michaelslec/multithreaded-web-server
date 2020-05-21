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

START_TEST(get_request_test)
{
    struct httpRequest get_goodfile = {"GET", "SMALL_TEST", "1.1", 0};
    ck_assert_int_eq(get_request(get_goodfile), 200);

    struct httpRequest get_nofile = {"GET", "TEST", "1.1", 0};
    ck_assert_int_eq(get_request(get_nofile), 404);

    struct httpRequest get_noperm = {"GET", "NOPERMISSION", "1.1", 0};
    ck_assert_int_eq(get_request(get_noperm), 403);
}
END_TEST

START_TEST(head_request_test)
{
    struct httpRequest head_goodfile = {"HEAD", "SMALL_TEST", "1.1", 0};
    ck_assert_int_eq(head_request(head_goodfile), 200);

    struct httpRequest head_nofile = {"HEAD", "TEST", "1.1", 0};
    ck_assert_int_eq(head_request(head_nofile), 404);

    struct httpRequest head_noperm = {"HEAD", "NOPERMISSION", "1.1", 0};
    ck_assert_int_eq(head_request(head_noperm), 200);
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

START_TEST(process_request_test)
{
    struct httpRequest get_goodfile = {"GET", "SMALL_TEST", "1.1", 0};
    struct httpResponse get_goodfile_res = {"GET", 200, "OK", 14};
    struct httpResponse get_goodfile_outcome = process_request(get_goodfile);
    ck_assert_mem_eq(&get_goodfile_outcome, &get_goodfile_res, sizeof(struct httpResponse));

    struct httpRequest get_nofile = {"GET", "TEST", "1.1", 0};
    struct httpResponse get_nofile_res = {"GET", 404, "Not Found", 0};
    struct httpResponse get_nofile_outcome = process_request(get_nofile);
    ck_assert_mem_eq(&get_nofile_outcome, &get_nofile_res, sizeof(struct httpResponse));

    struct httpRequest get_noperm = {"GET", "NOPERMISSION", "1.1", 0};
    struct httpResponse get_noperm_res = {"GET", 403, "Forbidden", 0};
    struct httpResponse get_noperm_outcome = process_request(get_noperm);
    ck_assert_mem_eq(&get_noperm_outcome, &get_noperm_res, sizeof(struct httpResponse));

    struct httpRequest put_goodfile = {"PUT", "PUT_TEST", "1.1", 11};
    struct httpResponse put_goodfile_res = {"PUT", 201, "Created", 0};
    struct httpResponse put_goodfile_outcome = process_request(put_goodfile); 
    ck_assert_mem_eq(&put_goodfile_outcome, &put_goodfile_res, sizeof(struct httpResponse));

    struct httpRequest head_goodfile = {"HEAD", "SMALL_TEST", "1.1", 0};
    struct httpResponse head_goodfile_res = {"HEAD", 200, "OK", 14};
    struct httpResponse head_goodfile_outcome = process_request(head_goodfile);
    ck_assert_mem_eq(&head_goodfile_outcome, &head_goodfile_res, sizeof(struct httpResponse));

    struct httpRequest head_nofile = {"HEAD", "TEST", "1.1", 0};
    struct httpResponse head_nofile_res = {"HEAD", 404, "Not Found", 0};
    struct httpResponse head_nofile_outcome = process_request(head_nofile);
    ck_assert_mem_eq(&head_nofile_outcome, &head_nofile_res, sizeof(struct httpResponse));

    struct httpRequest head_noperm = {"HEAD", "NOPERMISSION", "1.1", 0};
    struct httpResponse head_noperm_res = {"HEAD", 200, "OK", 0};
    struct httpResponse head_noperm_outcome = process_request(head_noperm);
    ck_assert_mem_eq(&head_noperm_outcome, &head_noperm_res, sizeof(struct httpResponse));
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
    tcase_add_test(tc_core, process_request_test);
    tcase_add_test(tc_core, get_request_test);
    tcase_add_test(tc_core, head_request_test);
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

