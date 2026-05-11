package com.zelgius.lightcontroller.ui.lights.details

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.domain.repository.web.StateMode
import com.zelgius.lightcontroller.domain.repository.web.WebItem
import com.zelgius.lightcontroller.domain.useCase.SaveItemsUseCase
import com.zelgius.lightcontroller.ui.common.ColorInfo
import com.zelgius.lightcontroller.ui.common.Light
import com.zelgius.lightcontroller.ui.common.TemperatureInfo
import com.zelgius.lightcontroller.ui.common.XYPoint
import com.zelgius.lightcontroller.ui.lights.add.State
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.items_update_failed
import lightcontroller.composeapp.generated.resources.items_update_success
import org.jetbrains.compose.resources.getString
import org.koin.core.annotation.KoinViewModel
import com.zelgius.lightcontroller.domain.repository.web.Light as WLight
import com.zelgius.lightcontroller.domain.repository.web.Switch as WSwitch

@KoinViewModel
class GroupDetailsViewModel(
    private val serverRepository: ServerRepository,
    private val saveItemsUseCase: SaveItemsUseCase
) : ViewModel() {
    private val _state = MutableStateFlow(GroupDetailsState())
    val state = _state.asStateFlow()

    private val _snackbarMessage = MutableStateFlow<String?>(null)
    val snackbarMessage = _snackbarMessage.asStateFlow()
    fun clearMessage() {
        _snackbarMessage.value = null
    }

    val addedLights
        get() = _state.value.items.filterIsInstance<Item.Light>().map {
            it.light
        }

    private lateinit var group: Group
    fun initialize(group: Group) {
        this.group = group
        _state.update { it.copy(group = group) }
        refresh()
    }

    fun refresh() = viewModelScope.launch {
        _state.update { it.copy(isLoading = true) }

        _state.update { state ->
            state.copy(isLoading = false, items = buildList {
                addAll(serverRepository.getSwitchesByGroup(groupId = group.id).map {
                    Item.Switch(it)
                })
                addAll(serverRepository.getLightsByGroup(group.id).map {
                    Item.Light(it)
                })
            })
        }
    }

    fun onGroupChanged(value: String) {
        _state.update {
            it.copy(group = it.group?.copy(name = value))
        }
    }

    fun onDelete(item: Item) {
        _state.update { s ->
            s.copy(items = s.items.map {
                if (it == item) {
                    val status =  if(it.status == Item.Status.Deleted) null else Item.Status.Deleted
                    when (it) {
                        is Item.Light -> it.copy(status = status)
                        is Item.Switch -> it.copy(status = status)
                    }
                } else it
            })
        }
    }

    fun onLightUpdate(light: Light, state: StateMode?) {
        _state.update { s ->
            s.copy(items = s.items.map {
                if (it is Item.Light && it.uid == light.id) {
                    val l = it.light
                    it.copy(
                        status = Item.Status.Updated,
                        light = it.light.copy(
                            brightness = light.dimming?.brightness ?: l.brightness,
                            x = light.color?.xy?.x ?: 0f,
                            y = light.color?.xy?.y ?: 0f,
                            mirek = light.colorTemperature?.mirek,
                            state = state ?: l.state
                        )
                    )
                } else it
            })
        }
    }

    fun onGroupColorUpdated(color: XYPoint) {
        _state.update {
            group = group.copy(x = color.x, y = color.y, brightness = color.brightness)
            it.copy(group = group)
        }
    }

    fun onItemsAdded(items: List<WebItem>) {
        _state.update {
            it.copy(items = it.items + items.map { item ->
                when (item) {
                    is WLight -> Item.Light(item, status = Item.Status.Added)
                    is WSwitch -> Item.Switch(item, status = Item.Status.Added)
                }
            }.distinctBy { item -> item.uid }
                .sortedWith { a, b ->
                    when (a) {
                        is Item.Light if b is Item.Switch -> -1
                        is Item.Switch if b is Item.Light -> 1
                        is Item.Light if b is Item.Light -> a.light.name.compareTo(b.light.name)
                        else -> a.uid.compareTo(b.uid)
                    }
                }
            )
        }
    }

    fun onSave() = viewModelScope.launch {
        val state = _state.value

        _state.update {
            it.copy(isLoading = true)
        }

        saveItemsUseCase(
            group = group,
            upsertItems = state.items.filter {
                it.added || it.updated
            }.toWebItems(),
            deletedItems = state.items.filter {
                it.deleted
            }.toWebItems()
        ).onFailure {
            _snackbarMessage.value = getString(Res.string.items_update_failed)
        } .onSuccess {
            _snackbarMessage.value = getString(Res.string.items_update_success)
        }

        refresh()

        _state.update {
            it.copy(isLoading = false)
        }

    }
}


sealed class Item(
    val uid: String,
    open val status: Status? = null,
) {

    val deleted: Boolean get() = status == Status.Deleted
    val updated: Boolean get() = status == Status.Updated
    val added: Boolean get() = status == Status.Added

    enum class Status {
        Added, Updated, Deleted
    }

    data class Light(
        val light: WLight,
        override val status: Status? = null,
    ) :
        Item(light.uid)

    data class Switch(
        val switch: WSwitch,
        override val status: Status? = null,
    ) :
        Item(switch.uid)


}

fun List<Item>.toWebItems(): List<WebItem> = map {
    when (it) {
        is Item.Switch -> it.switch
        is Item.Light -> it.light
    }
}

fun Item.toWebItem(): WebItem = when (this) {
    is Item.Switch -> switch
    is Item.Light -> light
}

data class GroupDetailsState(
    val isLoading: Boolean = true,
    val group: Group? = null,
    val items: List<Item> = emptyList()
)