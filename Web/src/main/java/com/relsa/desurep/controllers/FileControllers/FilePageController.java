package com.relsa.desurep.controllers.FileControllers;

import com.relsa.desurep.models.File;
import com.relsa.desurep.services.FileStorageService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.io.ByteArrayResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.servlet.mvc.support.RedirectAttributes;

import java.sql.Blob;
import java.sql.SQLException;
import java.util.Objects;

@Controller
public class FilePageController {
    @Autowired
    FileStorageService fileStorageService;

    @GetMapping("/openfile")
    public String viewFile(Model model,
                           @RequestParam String filename) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        File file = fileStorageService.getFile(filename, authentication.getName(), 0);
        if (file != null) {
            String link = "http://desurep.lol/download_file/?id=" + file.getDownloadId();
            model.addAttribute(file);
            model.addAttribute("link", link);
            model.addAttribute("description", file.getDescription());
            return "file_pages/file_page";
        }
        return "file_pages/file_error_page";
    }

    @GetMapping("/download_file")
    public ResponseEntity<Resource> downloadFile(@RequestParam String id) throws SQLException {
        File file = fileStorageService.getFileUnauth(id);
        Blob fileData = file.getData();
        int blobLength = (int) fileData.length();
        byte[] blobAsBytes = fileData.getBytes(1, blobLength);
        return ResponseEntity.ok()
                .contentType(MediaType.parseMediaType(file.getDocType()))
                .header(HttpHeaders.CONTENT_DISPOSITION, "attachment:" +
                        "filename=\"" + file.getPublicationName() + "\"")
                .body(new ByteArrayResource(blobAsBytes));
    }
}
