import {jsonBookProcessor} from "./controllers/search_api_controller.js";
import {getBookSearchApiUrl, hideLoader, showLoader, tableUpdate} from "./views/search_api_view.js";

const search_button = document.getElementById("global_search_button")
const search_table = document.getElementById("table_body");
const search_field = document.getElementById("global_search_field");

search_button.addEventListener("click", function (event) {
    showLoader(search_button);
    const apiUrl = getBookSearchApiUrl(search_field)
    if (apiUrl !== null) {
        apiProcessor(apiUrl, search_table, search_button);
    }
});

function apiProcessor(url, search_table, search_button) {
    fetch(url)
        .then( function( response ) { return response.json( ) } )
        .then( function( data ) { return jsonBookProcessor(data['docs']) } )
        .then(function (new_table_data) {tableUpdate(new_table_data, search_table)} )
        .then(function () { hideLoader(search_button) } )
        .catch( function( err ) { return console.log( err ) } )
}