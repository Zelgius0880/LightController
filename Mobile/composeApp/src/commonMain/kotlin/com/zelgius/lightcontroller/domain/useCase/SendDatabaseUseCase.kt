package com.zelgius.lightcontroller.domain.useCase

import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import io.github.vinceglb.filekit.PlatformFile
import io.github.vinceglb.filekit.readBytes
import io.ktor.http.HttpStatusCode
import org.koin.core.annotation.Factory
import org.koin.core.annotation.Provided

@Factory
class SendDatabaseUseCase(
    @Provided private val serverRepository: ServerRepository,
) {

    suspend operator fun invoke(file: PlatformFile): Boolean {
        val response = serverRepository.importDatabase(file.readBytes())

        return response.status == HttpStatusCode.OK
    }
}