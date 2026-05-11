package com.zelgius.lightcontroller.domain.useCase

import com.zelgius.lightcontroller.domain.repository.web.Group
import com.zelgius.lightcontroller.domain.repository.web.Light
import com.zelgius.lightcontroller.domain.repository.web.ServerRepository
import com.zelgius.lightcontroller.domain.repository.web.Switch
import com.zelgius.lightcontroller.domain.repository.web.WebItem
import io.ktor.http.HttpStatusCode
import org.koin.core.annotation.Factory

@Factory
class SaveItemsUseCase(
    private val serverRepository: ServerRepository
) {
    suspend operator fun invoke(
        group: Group,
        upsertItems: List<WebItem>,
        deletedItems: List<WebItem>
    ): Result<Unit> {
        tryBlock {
            val response = serverRepository.upsertGroup(group)
            if (response.status != HttpStatusCode.OK) error("Http failed: ${response.status}")
        }.onFailure {
            it.printStackTrace()
            return Result.failure(it)
        }

        upsertItems.forEach { i ->
            tryBlock {
                val response = when (i) {
                    is Light -> serverRepository.upsertLight(i.copy(groupId = group.id))
                    is Switch -> serverRepository.upsertSwitch(i.copy(groupId = group.id))
                }
                if (response.status != HttpStatusCode.OK) error("Http failed: ${response.status}")
            }.onFailure {
                it.printStackTrace()
            }
        }

        deletedItems.forEach { i ->
            tryBlock {
                val response = when (i) {
                    is Light -> serverRepository.deleteLight(i.uid, group.id)
                    is Switch -> serverRepository.deleteSwitch(i.uid)
                }
                if (response.status != HttpStatusCode.OK) error("Http failed: ${response.status}")
            }.onFailure {
                it.printStackTrace()
            }
        }

        return Result.success(Unit)
    }


    private suspend fun <T> tryBlock(block: suspend () -> T): Result<T> = try {
        val result = block()
        Result.success(result)
    } catch (e: Exception) {
        Result.failure(e)
    }
}