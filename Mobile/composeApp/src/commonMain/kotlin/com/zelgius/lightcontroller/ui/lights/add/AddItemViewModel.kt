package com.zelgius.lightcontroller.ui.lights.add

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.data.hue.LightResource
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.LightType
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.domain.repository.web.StateMode
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.repository.web.WebItem
import com.zelgius.lightcontroller.domain.useCase.GetLightsUseCase
import com.zelgius.lightcontroller.ui.home.HomeState
import com.zelgius.lightcontroller.utils.updateTo
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.this_group
import org.jetbrains.compose.resources.getString
import org.koin.core.annotation.KoinViewModel
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.Duration.Companion.seconds
import com.zelgius.lightcontroller.domain.repository.web.Switch as WSwitch

@KoinViewModel
class AddItemViewModel(
    val getLightsUseCase: GetLightsUseCase,
    val serverRepository: ServerRepository
) : ViewModel() {

    var group: Group? = null
    private set
    private val _state: MutableStateFlow<State> = MutableStateFlow(State.Loading)
    val state = _state.asStateFlow()

    private var clickedSwitchJob: Job? = null

    private val _snackbarMessage = MutableStateFlow<String?>(null)
    val snackbarMessage = _snackbarMessage.asStateFlow()
    private var closed = false

    private val _clickedSwitch = MutableStateFlow<ClickedSwitch?>(null)
    val clickedSwitch = _clickedSwitch.asStateFlow()

    val addedItems: List<WebItem> get() = (_state.value as? State.Loaded)?.addedItems?.map {
        when(it) {
            is Item.Switch -> it.switch
            is Item.Light -> with(it.light){
                Light(
                    uid = id,
                    name = metadata.name,
                    x = color?.xy?.x ?:0f,
                    y = color?.xy?.y ?:0f,
                    brightness = dimming?.brightness ?: 0f,
                    mirek = colorTemperature?.mirek,
                    groupId = group?.id ?: 0,
                    state = StateMode.TOGGLE,
                    type = LightType.HUE
                )
            }
        }
    }?: emptyList()

    fun clearMessage() {
        _snackbarMessage.value = null
    }

    fun MutableStateFlow<State>.updateLoaded(
        function: (State.Loaded) -> State
    ) = updateTo(createTo = { State.Loaded() }, function = function)

    fun initialize(group: Group, lights: List<Light>) = viewModelScope.launch {
        closed = false
        this@AddItemViewModel.group = group
        _state.update {
            State.Loading
        }

        val lights = getLightsUseCase(group, lights)

        _clickedSwitch.value = null
        _state.update {
            State.Loaded(lights = lights.map { l -> Item.Light(l) })
        }

        serverRepository.toggleAttributionMode(true)

        collectClickedSwitch()
    }

    fun close() = viewModelScope.launch {
        closed = true
        clickedSwitchJob?.cancel()
        serverRepository.toggleAttributionMode(false)
    }

    fun collectClickedSwitch() {
        clickedSwitchJob = viewModelScope.launch {
            while (!closed) {
                try {
                    delay(2.seconds)

                    val data = serverRepository.getAttributionData().last_data

                    val lastClicked = _clickedSwitch.value
                    if (data != null && lastClicked?.uid != data) {
                        val existingGroup = serverRepository.checkSwitch(data)

                        _clickedSwitch.value = ClickedSwitch(
                            uid = data,
                            name = "Switch $data",
                            existingGroup = existingGroup.name
                                ?: (state.value as? State.Loaded)?.let {
                                    it.addedItems.firstOrNull { item -> item is Item.Switch && item.uid == data }
                                        ?.let {
                                            getString(Res.string.this_group)
                                        }
                                }
                        )
                    }
                } catch (_: Exception) {/* no-op */
                }
            }
        }
    }

    override fun onCleared() {
        close()
        super.onCleared()
    }

    fun addLight(item: Item.Light) {
        _state.updateLoaded {
            State.Loaded(
                lights = it.lights.map { l ->
                    if (l.light.id == item.light.id) item.copy(added = true)
                    else l
                },
                addedSwitches = it.addedSwitches,
            )
        }
    }

    fun addSwitch(uid: String) {
        _state.updateLoaded {
            State.Loaded(
                lights = it.lights,
                addedSwitches = it.addedSwitches + Item.Switch(
                    Switch(
                        uid = uid,
                        name = "Switch $uid",
                        groupId = 0
                    ),
                ),
            )
        }

        viewModelScope.launch {
            _clickedSwitch.update {
                it?.copy(existingGroup = getString(Res.string.this_group))
            }
        }


    }


    fun removeLight(item: Item.Light) {
        _state.updateLoaded {
            State.Loaded(
                lights = it.lights.map { l ->
                    if (l.light.id == item.light.id) item.copy(added = false)
                    else l
                },
                addedSwitches = it.addedSwitches,
            )
        }
    }

    fun removeSwitch(item: Item.Switch) {
        _clickedSwitch.value = null
        _state.updateLoaded {
            State.Loaded(
                lights = it.lights,
                addedSwitches = it.addedSwitches.filter { s -> s.switch.uid != item.switch.uid },
            )
        }
    }


}

data class ClickedSwitch(
    val uid: String,
    val name: String,
    val existingGroup: String?
)

sealed interface State {
    data object Loading : State
    data class Loaded(
        val lights: List<Item.Light> = emptyList(),
        val addedSwitches: List<Item.Switch> = emptyList(),
        val addedLights: List<Item.Light> = lights.filter { it.added }.sortedBy {
            it.light.metadata.name
        },
        val availableLights: List<Item.Light> = lights.filter { !it.added }.sortedBy {
            it.light.metadata.name
        },
        val addedItems: List<Item> = addedSwitches + addedLights
    ) : State
}

sealed class Item(val uid: String) {
    data class Light(val light: LightResource, val added: Boolean = false) : Item(uid = light.id)
    data class Switch(val switch: WSwitch) : Item(uid = switch.uid)
}