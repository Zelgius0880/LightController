package com.zelgius.lightcontroller.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.settings.SettingsRepository
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class AppViewModel(
    settingsRepository: SettingsRepository
) : ViewModel() {
    val settingSet = combine(
        settingsRepository.ipFlow,
        settingsRepository.portFlow
    ) { ip, port ->
        !ip.isNullOrBlank() && port != null
    }.stateIn(
        scope = viewModelScope,
        started = SharingStarted.Companion.WhileSubscribed(5000), null
    )
}