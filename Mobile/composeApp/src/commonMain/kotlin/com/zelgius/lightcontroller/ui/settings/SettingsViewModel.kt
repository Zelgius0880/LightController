package com.zelgius.lightcontroller.ui.settings

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.settings.SettingsRepository
import com.zelgius.lightcontroller.domain.web.WebSocketRepository
import com.zelgius.lightcontroller.utils.updateTo
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.connection_failed
import lightcontroller.composeapp.generated.resources.connection_success
import org.jetbrains.compose.resources.getString
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class SettingsViewModel(
    private val settingsRepository: SettingsRepository,
    private val webSocketRepository: WebSocketRepository
) : ViewModel() {
    private val _snackbarMessage = MutableStateFlow<String?>(null)
    val snackbarMessage = _snackbarMessage.asStateFlow()
    fun clearMessage() {
        _snackbarMessage.value = null
    }

    fun MutableStateFlow<SettingState>.updateLoaded(
        function: (SettingState.Loaded) -> SettingState
    ) = updateTo(createTo = { SettingState.Loaded() }, function = function)

    private val _state: MutableStateFlow<SettingState> = MutableStateFlow(SettingState.Loading)
    val state = _state.asStateFlow()


    init {
        viewModelScope.launch {
            settingsRepository.ipFlow.collect { ip ->
                _state.updateLoaded {
                    it.copy(ip = ip)
                }
            }
        }
        viewModelScope.launch {
            settingsRepository.portFlow.collect { port ->
                _state.updateLoaded {
                    it.copy(port = port?: 80)
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
    }

    fun onIpChanged(newIp: String) {
        _state.updateLoaded {
            it.copy(ip = newIp)
        }
    }

    fun onPortChanged(newPort: String) {
        newPort.toIntOrNull()?.let { port ->
            _state.updateLoaded {
                it.copy(port = port)
            }
        }

    }

    fun testConnection() {
        val state = this.state.value
        if(state !is SettingState.Loaded) return

        _state.updateLoaded {
            it.copy(connected = null)
        }

        val (ip, port) = state
        if (!ip.isNullOrBlank()) {
            webSocketRepository.connectWs(ip, port) { connected ->
                viewModelScope.launch {
                    if(connected) {
                        _snackbarMessage.value = getString(Res.string.connection_success)
                        settingsRepository.saveIp(ip)
                        settingsRepository.savePort(port)
                    } else {
                        _snackbarMessage.value = getString(Res.string.connection_failed)
                    }
                }

                _state.updateLoaded {
                    it.copy(connected = connected)
                }
            }
        }
    }
}

sealed interface SettingState {
    data object Loading : SettingState
    data class Loaded(
        val ip: String? = null,
        val port: Int = 80,
        val connected: Boolean? = false
    ) : SettingState
}