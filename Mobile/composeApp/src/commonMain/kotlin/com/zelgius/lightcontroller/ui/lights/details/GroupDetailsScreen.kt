package com.zelgius.lightcontroller.ui.lights.details

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.expandHorizontally
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.shrinkHorizontally
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyItemScope
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.twotone.Add
import androidx.compose.material.icons.twotone.ColorLens
import androidx.compose.material.icons.twotone.Delete
import androidx.compose.material.icons.twotone.Lightbulb
import androidx.compose.material.icons.twotone.Restore
import androidx.compose.material.icons.twotone.Save
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.FilledIconButton
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.IconButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedIconButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshotFlow
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.LightType
import com.zelgius.lightcontroller.domain.repository.web.StateMode
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.repository.web.WebItem
import com.zelgius.lightcontroller.ui.common.AppPullToRefresh
import com.zelgius.lightcontroller.ui.common.XYPoint
import com.zelgius.lightcontroller.ui.common.mirekRange
import com.zelgius.lightcontroller.ui.common.toColor
import com.zelgius.lightcontroller.ui.common.toRgb
import com.zelgius.lightcontroller.ui.lights.groupColorPicker.UpdateGroupColorDialog
import com.zelgius.lightcontroller.ui.lights.lightColorPicker.UpdateLightDialog
import com.zelgius.lightcontroller.utils.getContrastColor
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.add
import lightcontroller.composeapp.generated.resources.color_palette
import lightcontroller.composeapp.generated.resources.delete
import lightcontroller.composeapp.generated.resources.name
import lightcontroller.composeapp.generated.resources.radio_button_checked
import lightcontroller.composeapp.generated.resources.radio_button_partial
import lightcontroller.composeapp.generated.resources.radio_button_unchecked
import lightcontroller.composeapp.generated.resources.save
import lightcontroller.composeapp.generated.resources.switch_24dp
import org.jetbrains.compose.resources.painterResource
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel

@Composable
fun GroupDetailsScreen(
    group: Group,
    viewModel: GroupDetailsViewModel = koinViewModel(),
    onAddItem: (List<Light>) -> Unit
) {

    var light: Light? by remember {
        mutableStateOf(null)
    }

    var showGroupColorPicker by remember { mutableStateOf(false) }

    val snackbarHostState = remember { SnackbarHostState() }

    val snackbarMessage by viewModel.snackbarMessage.collectAsState()

    // Listen for events from ViewModel
    LaunchedEffect(snackbarMessage) {
        snackbarMessage?.let { message ->
            snackbarHostState.showSnackbar(message)
            viewModel.clearMessage()
        }
    }

    val state by viewModel.state.collectAsState()
    GroupDetailsScreen(
        state = state,
        onRefresh = viewModel::refresh,
        onClick = {
            if (it is Item.Light) light = it.light
        },
        onDelete = viewModel::onDelete,
        snackbarHostState = snackbarHostState,
        onGroupNameChanged = viewModel::onGroupChanged,
        onColorClicked = { showGroupColorPicker = true },
        onAddItem = {
            onAddItem(viewModel.addedLights)
        },
        onSave = viewModel::onSave
    )

    light?.let {
        UpdateLightDialog(it, onDismiss = {
            light = null
        }, onSave = { l, status ->
            viewModel.onLightUpdate(l, status)
            light = null
        })
    }

    if (showGroupColorPicker) {
        UpdateGroupColorDialog(
            onDismiss = { showGroupColorPicker = false },
            group = group,
            onSave = {
                viewModel.onGroupColorUpdated(it)
                showGroupColorPicker = false
            }
        )
    }
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class, ExperimentalMaterial3Api::class)
@Composable
private fun GroupDetailsScreen(
    state: GroupDetailsState,
    snackbarHostState: SnackbarHostState = remember { SnackbarHostState() },
    onRefresh: () -> Unit,
    onClick: (Item) -> Unit,
    onDelete: (Item) -> Unit,
    onColorClicked: () -> Unit,
    onAddItem: () -> Unit,
    onGroupNameChanged: (String) -> Unit,
    onSave: () -> Unit,
) {
    val listState = rememberLazyListState()
    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) },
        topBar = {
            TopAppBar(title = {
                Row(
                    Modifier.padding(end = 8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedTextField(
                        value = state.group?.name ?: "", modifier = Modifier.weight(1f), label = {
                            Text(stringResource(Res.string.name))
                        }, onValueChange = onGroupNameChanged, shape = CircleShape
                    )

                    val group = state.group

                    if (group != null && (group.y > 0f && group.x != 0f || group.mirek != null)) {
                        val (_, _, brightness, x, y, mirek) = group

                        val color =
                            if (x == 0f && y == 0f && mirek == null) CardDefaults.cardColors().containerColor
                            else if (mirek != null && mirek in mirekRange) mirek.toColor()
                            else XYPoint(x, y, brightness).toRgb()

                        val animatedColor by animateColorAsState(color)
                        FilledIconButton(
                            onClick = onColorClicked,
                            modifier = Modifier.align(Alignment.CenterVertically),
                            colors = IconButtonDefaults.filledIconButtonColors(containerColor = animatedColor)
                        ) {
                            Icon(
                                Icons.TwoTone.ColorLens,
                                contentDescription = stringResource(Res.string.color_palette),
                                tint = color.getContrastColor()
                            )
                        }
                    } else {
                        OutlinedIconButton(onClick = onColorClicked) {
                            Icon(
                                Icons.TwoTone.ColorLens,
                                contentDescription = stringResource(Res.string.color_palette)
                            )
                        }
                    }

                    Button(onClick = onSave) {
                        Icon(
                            Icons.TwoTone.Save, contentDescription = stringResource(Res.string.save)
                        )
                    }

                }
            })
        },
        floatingActionButton = {
            val isScrolling by remember {
                derivedStateOf { listState.isScrollInProgress }
            }

            AnimatedVisibility(
                !isScrolling,
                enter = fadeIn() + scaleIn(),
                exit = fadeOut() + scaleOut()
            ) {
                FloatingActionButton(onClick = onAddItem) {
                    Icon(Icons.TwoTone.Add, contentDescription = stringResource(Res.string.add))
                }
            }
        }
    ) { padding ->
        AppPullToRefresh(
            modifier = Modifier.padding(padding).fillMaxSize(),
            isRefreshing = state.isLoading,
            onRefresh = onRefresh,
        ) {
            LazyColumn(
                modifier = Modifier.fillMaxSize().padding(8.dp),
                verticalArrangement = Arrangement.spacedBy(4.dp),
                state = listState
            ) {
                items(state.items, key = { it.uid }) {
                    when (it) {
                        is Item.Light -> LightItem(
                            it, onLightClicked = onClick, onLightDeleted = onDelete
                        )

                        is Item.Switch -> SwitchItem(
                            it, onSwitchClicked = onClick, onSwitchDeleted = onDelete
                        )
                    }
                }

                item {
                    Box(modifier = Modifier.height(64.dp))
                }
            }
        }
    }
}


@Composable
private fun LazyItemScope.LightItem(
    item: Item.Light, onLightClicked: (Item.Light) -> Unit, onLightDeleted: (Item.Light) -> Unit
) {
    val light = item.light
    val container =
        if (light.x == 0f && light.y == 0f && light.mirek == null) CardDefaults.cardColors().containerColor
        else if (light.mirek != null && light.mirek in mirekRange) light.mirek.toColor()
        else XYPoint(light.x, light.y, light.brightness).toRgb()

    val animatedAlpha by animateFloatAsState(if (item.deleted) 0.4f else 1f)

    Card(
        modifier = Modifier.animateItem().fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = container),
        onClick = { onLightClicked(item) }) {
        val color = container.getContrastColor().copy(alpha = animatedAlpha)
        Row(modifier = Modifier.padding(8.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(modifier = Modifier.fillMaxHeight().weight(1f)) {
                Row(
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(Icons.TwoTone.Lightbulb, contentDescription = null, tint = color)
                    Text(
                        light.name,
                        color = color,
                        modifier = Modifier.weight(1f).padding(horizontal = 8.dp)
                    )
                    Icon(
                        painterResource(
                            when (light.state) {
                                StateMode.TOGGLE -> Res.drawable.radio_button_partial
                                StateMode.ON -> Res.drawable.radio_button_checked
                                StateMode.OFF -> Res.drawable.radio_button_unchecked
                            }
                        ), contentDescription = null, tint = color
                    )

                }
                androidx.compose.animation.AnimatedVisibility(
                    item.deleted,
                    enter = fadeIn() + expandHorizontally(),
                    exit = fadeOut() + shrinkHorizontally(),
                    modifier = Modifier.fillMaxHeight().align(Alignment.Center)
                ) {
                    Box(
                        Modifier.fillMaxWidth().height(1.dp).background(color)
                    )
                }
            }
            FilledIconButton(
                onClick = {
                    onLightDeleted(item)
                },
                colors = IconButtonDefaults.iconButtonColors()
                    .copy(containerColor = MaterialTheme.colorScheme.errorContainer)
            ) {
                if (item.deleted) Icon(
                    Icons.TwoTone.Restore, contentDescription = stringResource(Res.string.delete)
                )
                else Icon(
                    Icons.TwoTone.Delete, contentDescription = stringResource(Res.string.delete)
                )
            }
        }
    }
}

@Composable
private fun LazyItemScope.SwitchItem(
    item: Item.Switch,
    onSwitchClicked: (Item.Switch) -> Unit,
    onSwitchDeleted: (Item.Switch) -> Unit
) {
    val switch = item.switch

    val animatedAlpha by animateFloatAsState(if (item.deleted) 0.4f else 1f)

    Card(
        modifier = Modifier.animateItem().fillMaxWidth(), onClick = { onSwitchClicked(item) }) {
        Row(modifier = Modifier.padding(8.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(modifier = Modifier.fillMaxHeight().weight(1f)) {
                Row(
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(
                        painterResource(Res.drawable.switch_24dp),
                        contentDescription = null,
                        tint = MaterialTheme.colorScheme.onSurface.copy(alpha = animatedAlpha)
                    )
                    Text(
                        switch.name,
                        modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                        color = MaterialTheme.colorScheme.onSurface.copy(alpha = animatedAlpha)
                    )
                }

                androidx.compose.animation.AnimatedVisibility(
                    item.deleted,
                    enter = fadeIn() + expandHorizontally(),
                    exit = fadeOut() + shrinkHorizontally(),
                    modifier = Modifier.fillMaxHeight().align(Alignment.Center)
                ) {
                    Box(
                        Modifier.fillMaxWidth().height(1.dp)
                            .background(MaterialTheme.colorScheme.onSurface)
                    )
                }
            }

            IconButton(
                onClick = {
                    onSwitchDeleted(item)
                },
                colors = IconButtonDefaults.iconButtonColors()
                    .copy(containerColor = MaterialTheme.colorScheme.errorContainer)
            ) {
                if (item.deleted) Icon(
                    Icons.TwoTone.Restore, contentDescription = stringResource(Res.string.delete)
                )
                else Icon(
                    Icons.TwoTone.Delete, contentDescription = stringResource(Res.string.delete)
                )
            }
        }
    }
}

@Preview
@Composable
private fun ItemPreview() {
    val light = Light(
        uid = "1",
        name = "Kitchen Light",
        state = StateMode.TOGGLE,
        brightness = 1f,
        x = 0.4f,
        y = 0.4f,
        mirek = null,
        type = LightType.HUE,
        groupId = 5
    )

    val switch = Switch(
        name = "Swtich 123456789", groupId = 5, uid = "165552326512"
    )

    MaterialTheme {
        GroupDetailsScreen(
            state = GroupDetailsState(
                group = Group(5, "Preview group", 0f, 0f, 0f), items = listOf(
                    Item.Switch(switch, status = Item.Status.Deleted), Item.Light(light)
                )
            ),
            onClick = {},
            onRefresh = {},
            onDelete = {},
            onGroupNameChanged = {},
            onColorClicked = {},
            onAddItem = {},
            onSave = {},
        )
    }
}