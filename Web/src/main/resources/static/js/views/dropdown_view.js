function dropdownFunct() {
    document.getElementById("profile_dropdown").classList.toggle("show");
}

<!-- Close -->
window.onclick = function(event) {
    if (!event.target.matches('.drop_btn')) {
        let dropdowns = document.getElementsByClassName("profile_dropdown");
        for (let i = 0; i < dropdowns.length; i++) {
            let openDropdown = dropdowns[i];
            if (openDropdown.classList.contains('show')) {
                openDropdown.classList.remove('show');
            }
        }
    }
}