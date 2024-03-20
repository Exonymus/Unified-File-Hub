import {activateButtons, disableErrorClass, setErrorClass} from "./controllers/form_validation_controller.js";

const email = document.getElementById("floating_input_email");
const password = document.getElementById("floating_password");
const password_conf = document.getElementById("floating_password_repeat");
const sub_but = document.getElementById("sub_but");

const email_div = document.getElementById("email_div");
const password_div = document.getElementById("password_div");
const password_conf_div = document.getElementById("passwordSec_div");

email.addEventListener("input", function (event) {
    const email_format = /.+@.+\..+/;
    if (!email_format.test(email.value)) {
        setErrorClass(email_div, sub_but, "email");
    } else {
        disableErrorClass(email_div, "email")
    }
    activateButtons(sub_but);
});

password.addEventListener("input", function (event) {
    /*const password_format = /^(?=.*[0-9])(?=.*[!@#$%^&*])[a-zA-Z0-9!@#$%^&*]{6,16}$/;
    if (!password_format.test(password.value)) {
        setErrorClass(password_div, sub_but, "password");
    } else {
        disableErrorClass(password_div, "password");
    }*/

    disableErrorClass(password_div, "password");
    activateButtons(sub_but);
});

password_conf.addEventListener("input", function (event) {
    if (!Object.is(password.value, password_conf.value)) {
        setErrorClass(password_conf_div, sub_but, "password_conf")
    } else {
        disableErrorClass(password_conf_div, "password_conf")
    }
    activateButtons(sub_but);
});