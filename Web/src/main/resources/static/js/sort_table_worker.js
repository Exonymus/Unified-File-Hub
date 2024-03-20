import {getSort} from "./controllers/sort_table_controller.js";

document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('.table_sort thead').forEach(tableTH =>
        tableTH.addEventListener('click', () => getSort(event)));
});