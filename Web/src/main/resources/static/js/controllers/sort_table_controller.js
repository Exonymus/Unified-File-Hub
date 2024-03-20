import {appendSortRows, cellUpdate} from "../views/sort_table_view.js";

export function getSort ({ target }) {
    const order = (target.dataset.order = -(target.dataset.order || -1));
    const index = [...target.parentNode.cells].indexOf(target);
    const collator = new Intl.Collator(['en', 'ru'], { numeric: true });
    const comparator = (index, order) => (a, b) => order * collator.compare(
        a.children[index].innerHTML,
        b.children[index].innerHTML
    );

    appendSortRows({ target }, order, index, comparator);
    cellUpdate({ target });
}