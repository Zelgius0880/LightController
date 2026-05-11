package com.zelgius.lightcontroller.ui.lights.add

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.twotone.Add
import androidx.compose.material.icons.twotone.Close
import androidx.compose.material.icons.twotone.Lightbulb
import androidx.compose.material.icons.twotone.Save
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.FilledIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.InputChip
import androidx.compose.material3.LoadingIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.data.hue.LightResource
import com.zelgius.lightcontroller.data.hue.Metadata
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.repository.web.WebItem
import kotlinx.coroutines.delay
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.add_items_to
import lightcontroller.composeapp.generated.resources.already_added_in
import lightcontroller.composeapp.generated.resources.delete
import lightcontroller.composeapp.generated.resources.last_click_switch
import lightcontroller.composeapp.generated.resources.save
import lightcontroller.composeapp.generated.resources.switch_24dp
import org.jetbrains.compose.resources.painterResource
import org.jetbrains.compose.resources.stringResource
import org.koin.compose.viewmodel.koinViewModel
import kotlin.time.Duration.Companion.milliseconds

@Composable
fun AddItemScreen(
    group: Group,
    lights: List<Light>,
    viewModel: AddItemViewModel = koinViewModel(),
    onSave: (List<WebItem>) -> Unit
) {
    DisposableEffect(group) {
        viewModel.initialize(group, lights)

        onDispose {
            viewModel.close()
        }
    }

    val clickedSwitch by viewModel.clickedSwitch.collectAsState()
    val state by viewModel.state.collectAsState()

    AddItemScreen(
        groupName = viewModel.group?.name ?: "",
        state = state,
        clickedSwitch = clickedSwitch,
        onSwitchAdded = viewModel::addSwitch,
        onRemove = {
            when (it) {
                is Item.Light -> viewModel.removeLight(it)
                is Item.Switch -> viewModel.removeSwitch(it)
            }
        },
        onClicked = viewModel::addLight,
        onSave = {
            onSave(viewModel.addedItems)
        }
    )
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class, ExperimentalMaterial3Api::class)
@Composable
private fun AddItemScreen(
    groupName: String,
    state: State,
    clickedSwitch: ClickedSwitch?,
    onRemove: (item: Item) -> Unit,
    onClicked: (item: Item.Light) -> Unit,
    onSwitchAdded: (uid: String) -> Unit,
    onSave: () -> Unit = {}
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(stringResource(Res.string.add_items_to, groupName))
                },
                actions = {
                    Button(onClick = onSave) {
                        Icon(
                            Icons.TwoTone.Save, contentDescription = stringResource(Res.string.save)
                        )
                    }
                }
            )
        }
    ) { padding ->
        Box(Modifier.padding(padding).padding(8.dp)) {
            LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                item {
                    FlowRow(
                        horizontalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        (state as? State.Loaded)?.addedItems?.forEach {
                            AnimatedChip(it, onRemove = onRemove)
                        }
                    }
                }

                item(key = clickedSwitch?.uid) {
                    if (clickedSwitch != null) {
                        val interactionSource = remember { MutableInteractionSource() }
                        Card(
                            Modifier.fillMaxSize().animateItem()
                                .clickable(
                                    enabled = clickedSwitch.existingGroup.isNullOrBlank(),
                                    interactionSource = interactionSource,
                                    onClick = {  onSwitchAdded(clickedSwitch.uid)  })
                        ) {
                            Row(modifier = Modifier.padding(vertical = 8.dp, horizontal = 16.dp)) {
                                Column(modifier = Modifier.weight(1f)) {
                                    Text(stringResource(Res.string.last_click_switch), style = MaterialTheme.typography.labelSmall)
                                    Text(clickedSwitch.name)
                                    if (!clickedSwitch.existingGroup.isNullOrBlank()) {
                                        Text(
                                            "${stringResource(Res.string.already_added_in)} ${clickedSwitch.existingGroup}",
                                            color = MaterialTheme.colorScheme.error
                                        )
                                    }
                                }

                                AnimatedVisibility(clickedSwitch.existingGroup.isNullOrBlank()) {
                                    FilledIconButton(
                                        enabled = clickedSwitch.existingGroup.isNullOrBlank(),
                                        interactionSource = interactionSource,
                                        onClick = { onSwitchAdded(clickedSwitch.uid) }) {
                                        Icon(Icons.TwoTone.Add, contentDescription = null)
                                    }
                                }
                            }
                        }
                    }
                }

                (state as? State.Loaded)?.availableLights?.let { lights ->
                    items(lights, key = { it.light.id }) {
                        Card(
                            Modifier.fillMaxSize().animateItem()
                                .clickable(onClick = { onClicked(it) })
                        ) {
                            Text(
                                it.light.metadata.name,
                                modifier = Modifier.padding(vertical = 8.dp, horizontal = 16.dp)
                            )
                        }
                    }
                }
            }

            if (state is State.Loading) Box(modifier = Modifier.fillMaxWidth()) {
                LoadingIndicator(Modifier.align(Alignment.TopCenter))
            }
        }
    }
}


@Composable
fun AnimatedChip(item: Item, onRemove: (item: Item) -> Unit) {
    var visible by remember { mutableStateOf(false) }

    LaunchedEffect(visible) {
        if (!visible) {
            delay(200.milliseconds)
            onRemove(item)
        }
    }


    LaunchedEffect(item) {
        visible = true
    }

    AnimatedVisibility(
        visible, enter = fadeIn(tween(200)) + scaleIn(tween(200)),
        exit = scaleOut(tween(200)) + fadeOut(tween(200)),
    ) {
        InputChip(
            selected = false, onClick = {}, leadingIcon = {
                when (item) {
                    is Item.Light -> Icon(
                        Icons.TwoTone.Lightbulb,
                        contentDescription = null
                    )

                    is Item.Switch -> Icon(
                        painterResource(Res.drawable.switch_24dp),
                        contentDescription = null
                    )
                }
            }, label = {
                Text(
                    when (item) {
                        is Item.Light -> item.light.metadata.name
                        is Item.Switch -> item.switch.name
                    }
                )
            },
            trailingIcon = {
                Icon(
                    Icons.TwoTone.Close,
                    contentDescription = stringResource(Res.string.delete),
                    modifier = Modifier.clip(
                        CircleShape
                    ).clickable(onClick = { visible = false })
                )
            })
    }

}


@Preview(showBackground = true)
@Composable
fun PreviewAddSwitchAndLightScreen() {
    // 1. Create Mock Data
    val mockItems = listOf(
        Item.Light(LightResource("1", metadata = Metadata("Living Room Ceiling"))),
        Item.Switch(Switch("885332554", groupId = 1, name = "Master Bedroom Switch")),
        Item.Light(LightResource("2", metadata = Metadata("Kitchen Island")))
    )

    val mockAvailableLights = listOf(
        LightResource("3", metadata = Metadata("Desk Lamp")),
        LightResource("4", metadata = Metadata("Bedside Lamp Left")),
        LightResource("5", metadata = Metadata("Bedside Lamp Right")),
        LightResource("6", metadata = Metadata("Hallway Light"))
    )

    val mockState = State.Loaded(
        addedItems = mockItems,
        availableLights = mockAvailableLights.map { Item.Light(it) }
    )

    // 2. Render within a Theme
    MaterialTheme {
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            AddItemScreen(
                groupName =  "Test Group",
                state = mockState,
                clickedSwitch = ClickedSwitch("123", "Switch 123", null),
                onSwitchAdded = {},
                onRemove = { /* No-op for preview */ },
                onClicked = {}
            )
        }
    }
}
