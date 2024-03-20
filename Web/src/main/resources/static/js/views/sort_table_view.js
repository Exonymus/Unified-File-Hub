export function appendSortRows ({ target }, order, index, comparator) {
    for (const tBody of target.closest('table').tBodies)
        tBody.append(...[...tBody.rows].sort(comparator(index, order)));
}

export function cellUpdate({ target }) {
    for (const cell of target.parentNode.cells)
        cell.classList.toggle('sorted', cell === target);
}