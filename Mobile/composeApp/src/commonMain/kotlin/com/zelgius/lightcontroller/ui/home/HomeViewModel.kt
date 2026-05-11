package com.zelgius.lightcontroller.ui.home

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.repository.settings.SettingsRepository
import com.zelgius.lightcontroller.domain.repository.web.WebSocketMessage
import com.zelgius.lightcontroller.domain.repository.web.WebSocketRepository
import com.zelgius.lightcontroller.utils.updateTo
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class HomeViewModel(
    private val webSocketRepository: WebSocketRepository,
    private val settingsRepository: SettingsRepository
) : ViewModel() {
    private val _state: MutableStateFlow<HomeState> = MutableStateFlow(HomeState.Loading)
    val state = _state.asStateFlow()

    fun MutableStateFlow<HomeState>.updateLoaded(
        function: (HomeState.Loaded) -> HomeState
    ) = updateTo(createTo = { HomeState.Loaded() }, function = function)

    fun init() {

        viewModelScope.launch {
            webSocketRepository.wsMessageFlow.collect { m ->
                _state.updateLoaded {
                    when (m) {
                        is WebSocketMessage.Log -> {
                            it.copy(logs = buildList {
                                add(m)
                                addAll(it.logs)
                            })
                        }

                        is WebSocketMessage.Status -> {
                            it.copy(status = m)
                        }
                    }

                }
            }
        }

        viewModelScope.launch {
            webSocketRepository.wsConnectedFlow.collect { connected ->
                _state.updateLoaded {
                    it.copy(connected = connected)
                }
            }
        }

        viewModelScope.launch {
            combine(
                settingsRepository.ipFlow,
                settingsRepository.portFlow
            ) { ip, port ->
                !ip.isNullOrBlank() && port != null
            }.collect { set ->
                _state.updateLoaded {
                    it.copy(settingsSet = set)
                }
            }
        }

        connect()
    }

    fun connect() = viewModelScope.launch {
        tryConnect()
    }

    private suspend fun tryConnect() {
        val ip = settingsRepository.ipFlow.first()
        val port = settingsRepository.portFlow.first()
        if (ip != null && port != null) {
            webSocketRepository.connectWs(ip, port)
        }
    }
}


sealed interface HomeState {
    data object Loading : HomeState
    data class Loaded(
        val logs: List<WebSocketMessage.Log> = listOf(),
        val status: WebSocketMessage.Status = WebSocketMessage.Status(),
        val connected: Boolean = false,
        val settingsSet: Boolean? = null
    ) : HomeState

}
