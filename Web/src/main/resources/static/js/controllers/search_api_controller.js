export function jsonBookProcessor(json) {
    let new_table_data = "";
    json.map( book => {
        if (book.title !== undefined && book['author_name'] !== undefined && book['subject'] !== undefined
            && book.first_publish_year !== undefined) {
            new_table_data = apiBookParser(book, new_table_data);
        }
    });
    return new_table_data;
}

function apiBookParser(book, table) {
    table += '<tr>' + '<th class="fileTableBox">' + book.title + '</th>' +
        '<th class="file_table_box">' + Object.values(book['author_name'])[0] + '</th>' +
        '<th class="file_table_box">' + Object.values(book['subject'])[1] + '</th>' +
        '<th class="file_table_box">' + book.first_publish_year + '</th>' +
        '<th class="file_table_box"> - </th>' +
        '<th class="file_table_box"> ' +
        '<button class="table_btn">' +
        '<a target="_blank" href="https://openlibrary.org' + book.key +  '">' +
        '<img src="/img/icons/eye-fill.svg" alt="open">' +
        '</a>' +
        '</button>' +
        '</th>' +
        '</tr>';
    return table;
}

