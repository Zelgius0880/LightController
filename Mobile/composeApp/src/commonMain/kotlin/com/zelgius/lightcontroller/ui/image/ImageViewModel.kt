package com.zelgius.lightcontroller.ui.image

import androidx.lifecycle.ViewModel
import com.zelgius.lightcontroller.domain.useCase.JjnDithererUseCase
import org.koin.core.annotation.KoinViewModel

@KoinViewModel
class ImageViewModel(
    val jjnDithererUseCase: JjnDithererUseCase,
) : ViewModel() {

}