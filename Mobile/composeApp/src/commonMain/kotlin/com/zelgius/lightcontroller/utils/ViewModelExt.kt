package com.zelgius.lightcontroller.utils

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.update


inline fun <From, reified To : From> MutableStateFlow<From>.updateTo(
    createTo: (From) -> To,
    function: (To) -> From
) {
    this.update {
        val state = if (it !is To) createTo(it) else it
        function(state)
    }
}
