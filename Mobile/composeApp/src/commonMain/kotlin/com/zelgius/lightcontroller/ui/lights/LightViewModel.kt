package com.zelgius.lightcontroller.ui.lights

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class LightViewModel(
    private val serverRepository: ServerRepository
) : ViewModel() {

    private val _state = MutableStateFlow(LightState())
    val state = _state.asStateFlow()

    fun initialize() {
        refresh()
    }

    fun refresh() = viewModelScope.launch {
        try {
            _state.update { it.copy(isLoading = true) }
            val list = serverRepository.getAllGroups()
            _state.update { it.copy(isLoading = false, groups = list) }
        } catch (e: Exception) {
            _state.update { it.copy(isLoading = false) }
            e.printStackTrace()
        }

    }

    fun deleteGroup(group: Group) = viewModelScope.launch {
        try {
            _state.update { it.copy(isLoading = true) }
            serverRepository.deleteGroup(group.id)
            refresh()
        } catch (e: Exception) {
            _state.update { it.copy(isLoading = false) }
            e.printStackTrace()
        }
    }
}

data class LightState(
    val isLoading: Boolean = true,
    val groups: List<Group> = emptyList()
)