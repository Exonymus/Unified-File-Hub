<!-- Hide preloader when page load -->
window.onload = (event) => {
    document.getElementById("loader_wrapper").style.visibility = 'hidden';
};

<!-- Show preloader before redirect-->
window.onbeforeunload = function (event) {
    document.getElementById("loader_wrapper").style.visibility = 'visible';
};

<!-- Hide preloader when page redirect and save in cache (for Chrome)-->
window.onpagehide = (event) => {
    if (event.persisted) {
    document.getElementById("loader_wrapper").style.visibility = 'hidden';
    }
};

<!-- Hide preloader when page redirect and save in cache (for Firefox)-->
document.addEventListener("visibilitychange", () => {
    if (document.visibilityState !== "visible") {
        document.getElementById("loader_wrapper").style.visibility = 'hidden';
    }
});
