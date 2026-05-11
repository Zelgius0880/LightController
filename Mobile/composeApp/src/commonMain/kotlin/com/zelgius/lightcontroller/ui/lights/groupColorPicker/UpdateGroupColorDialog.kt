@file:OptIn(ExperimentalMaterial3ExpressiveApi::class)

package com.zelgius.lightcontroller.ui.lights.groupColorPicker

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.size
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonColors
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.ui.common.BrightnessSlider
import com.zelgius.lightcontroller.ui.common.HueGamutPicker
import com.zelgius.lightcontroller.ui.common.XYPoint
import com.zelgius.lightcontroller.ui.common.toRgb
import lightcontroller.composeapp.generated.resources.Res
import lightcontroller.composeapp.generated.resources.clear
import lightcontroller.composeapp.generated.resources.save
import org.jetbrains.compose.resources.stringResource

@Composable
fun UpdateGroupColorDialog(
    group: Group,
    onDismiss: () -> Unit,
    onSave: (XYPoint) -> Unit,
) {

    var currentColor by remember {
        mutableStateOf(XYPoint(group.x, group.y, group.brightness))
    }

    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = {
            Row (horizontalArrangement = Arrangement.spacedBy(8.dp)){
                OutlinedButton(
                    onClick = { onSave(XYPoint(0f, 0f, 0f)) },
                    border = BorderStroke(width = 2.dp, color = MaterialTheme.colorScheme.errorContainer),

                ) {
                    Text(stringResource(Res.string.clear))
                }

                Button(
                    onClick = { onSave(currentColor) },
                ) {
                    Text(stringResource(Res.string.save))
                }

            }
        },
        text = {
            Column {
                HueGamutPicker(
                    gamut = "A",
                    color = currentColor.toRgb(),
                    onColorSelected = {
                        currentColor = it
                    },
                    modifier = Modifier.size(300.dp)
                )

                BrightnessSlider(currentColor.brightness, onValueChange = {
                    currentColor = currentColor.copy(brightness = it)
                })
            }

        }
    )
}
