export function tableUpdate(table, search_table) {
    search_table.replaceChildren();
    search_table.insertAdjacentHTML('beforeEnd', table);
}

export function hideLoader(search_button) {
    search_button.classList.remove("button--loading");
    search_button.style.color = "white";
}

export function showLoader(search_button) {
    search_button.classList.toggle("button--loading");
    search_button.style.color = "#222";
}

export function getBookSearchApiUrl(search_field) {
    let query = search_field.value;
    if (query && query !== "") {
        query = query.replace(/ /gi, '+');
        return 'https://openlibrary.org/search.json?title=' + query;
    }
    return null;
}