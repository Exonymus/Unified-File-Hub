let email_fail_flag = false;
let password_fail_flag = false;
let password_conf_fail_flag = true;

export function activateButtons(sub_but) {
    if (!email_fail_flag && !password_fail_flag && !password_conf_fail_flag) {
        sub_but.removeAttribute("disabled");
    }
}

export function setErrorClass(elem, sub_but, flag_name) {
    elem.className="txt_field_error";
    sub_but.setAttribute("disabled", "");

    switch (flag_name) {
        case 'email':
            email_fail_flag = true;
            break;
        case 'password':
            password_fail_flag = true;
            break;
        case 'password_conf':
            password_conf_fail_flag = true;
            break;
    }
}

export function disableErrorClass(elem, flag_name) {
    elem.className="txt_field";

    switch (flag_name) {
        case 'email':
            email_fail_flag = false;
            break;
        case 'password':
            password_fail_flag = false;
            break;
        case 'password_conf':
            password_conf_fail_flag = false;
            break;
    }
}