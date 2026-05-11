@file:OptIn(ExperimentalMaterial3ExpressiveApi::class)

package com.zelgius.lightcontroller.ui.lights.lightColorPicker

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.CircularWavyProgressIndicator
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.repository.web.StateMode
import com.zelgius.lightcontroller.domain.useCase.GetLightUseCase
import com.zelgius.lightcontroller.ui.common.ColorInfo
import com.zelgius.lightcontroller.ui.common.HueLightPicker
import com.zelgius.lightcontroller.ui.common.Light
import com.zelgius.lightcontroller.ui.common.XYPoint
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.light_not_found
import lightcontroller.composeapp.generated.resources.off
import lightcontroller.composeapp.generated.resources.on
import lightcontroller.composeapp.generated.resources.save
import lightcontroller.composeapp.generated.resources.state_mode
import lightcontroller.composeapp.generated.resources.toggle
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel
import org.koin.core.annotation.KoinViewModel

@Composable
fun UpdateLightDialog(
    light: com.zelgius.lightcontroller.domain.repository.web.Light,
    viewModel: UpdateLightDialogViewModel = koinViewModel(),
    onDismiss: () -> Unit,
    onSave: (Light, StateMode) -> Unit
) {
    val state by viewModel.state.collectAsState()

    LaunchedEffect(light) {
        viewModel.initialize(light)
    }

    UpdateLightDialog(
        state,
        onDismiss,
        onSave = {
            (state as? State.Loaded)?.let { (light, stateMode) ->
                onSave(light, stateMode)
            }
        },
        onStateSelected = viewModel::onStateModeChanged,
        onLightUpdated = viewModel::onLightUpdated
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun UpdateLightDialog(
    state: State,
    onDismiss: () -> Unit,
    onSave: () -> Unit,
    onLightUpdated: (Light) -> Unit,
    onStateSelected: (StateMode) -> Unit
) {
    val light = (state as? State.Loaded)?.light

    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = {
            Button(onClick = onSave, enabled = light != null) {
                Text(stringResource(Res.string.save))
            }
        },
        title = {
            (state as? State.Loaded)?.let {
                Text(it.name)
            }
        },
        text = {
            when (state) {
                is State.Loading -> Box(Modifier.fillMaxWidth().aspectRatio(1f)) {
                    CircularWavyProgressIndicator(Modifier.align(Alignment.Center))
                }

                is State.NotFound -> Box(Modifier.fillMaxWidth().aspectRatio(1f)) {
                    Text(
                        stringResource(Res.string.light_not_found),
                        style = MaterialTheme.typography.bodyMedium.copy(color = MaterialTheme.colorScheme.error)
                    )
                }

                is State.Loaded -> {
                    light?.let { l ->
                        Column {
                            StateDropDown(
                                selected = state.stateMode,
                                onStateSelected = onStateSelected
                            )

                            HueLightPicker(
                                l,
                                onXYUpdate = {
                                    onLightUpdated(
                                        l.copy(
                                            color = l.color?.copy(xy = it),
                                            colorTemperature = l.colorTemperature?.copy(mirek = null)
                                        )
                                    )
                                },
                                onMirekUpdate = {
                                    onLightUpdated(
                                        l.copy(
                                            colorTemperature = l.colorTemperature?.copy(mirek = it),
                                            color = l.color?.copy(
                                                xy = XYPoint(
                                                    0f,
                                                    0f,
                                                    l.color.xy.brightness
                                                )
                                            )
                                        )
                                    )
                                },
                                onBrightnessUpdate = {
                                    onLightUpdated(
                                        l.copy(dimming = l.dimming?.copy(brightness = it))
                                    )
                                }
                            )
                        }
                    }
                }
            }
        }
    )
}


@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun StateDropDown(selected: StateMode, onStateSelected: (StateMode) -> Unit) {
    var expanded by remember { mutableStateOf(false) }
    ExposedDropdownMenuBox(expanded = expanded, onExpandedChange = {
        expanded = true
    }) {
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = !expanded }
        ) {

            OutlinedTextField(
                shape = CircleShape,
                label = { Text(stringResource(Res.string.state_mode)) },
                value = selected.string,
                onValueChange = {},
                readOnly = true,
                trailingIcon = {
                    ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded)
                },
                colors = ExposedDropdownMenuDefaults.outlinedTextFieldColors(),
                modifier = Modifier
                    .fillMaxWidth()
                    .menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
            )

            ExposedDropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false }
            ) {
                StateMode.entries.forEach { option ->
                    DropdownMenuItem(
                        text = { Text(option.string, color = MaterialTheme.colorScheme.onSurface) },
                        onClick = {
                            onStateSelected(option)
                            expanded = false
                        },
                        contentPadding = ExposedDropdownMenuDefaults.ItemContentPadding
                    )
                }
            }
        }
    }
}

@KoinViewModel
class UpdateLightDialogViewModel(
    private val getLightUseCase: GetLightUseCase,
) : ViewModel() {
    private val _state = MutableStateFlow<State>(State.Loading)
    val state = _state.asStateFlow()

    fun initialize(light: com.zelgius.lightcontroller.domain.repository.web.Light) =
        viewModelScope.launch {
            _state.update { State.Loading }
            val hueLight = getLightUseCase(light.uid)
            _state.update {
                hueLight?.let {
                    State.Loaded(it, light.state, light.name)
                } ?: State.NotFound
            }
        }

    fun onStateModeChanged(stateMode: StateMode) {
        _state.update {
            if (it !is State.Loaded) return
            it.copy(stateMode = stateMode)
        }
    }

    fun onLightUpdated(light: Light) {
        _state.update {
            if (it !is State.Loaded) return
            it.copy(light = light)
        }
    }
}


sealed interface State {
    data object Loading : State
    data object NotFound : State
    data class Loaded(val light: Light, val stateMode: StateMode, val name: String) : State
}

val StateMode.string
    @Composable
    get() = stringResource(
        when (this) {
            StateMode.TOGGLE -> Res.string.toggle
            StateMode.ON -> Res.string.on
            StateMode.OFF -> Res.string.off
        }
    )