package com.relsa.desurep.controllers.FileControllers;

import com.relsa.desurep.services.FileStorageService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.multipart.MultipartFile;

@Controller
public class FileUploadController {
    @Autowired
    private FileStorageService fileStorageService;

    @GetMapping("/upload_file")
    public String mapUpload(){
        return "file_pages/file_upload_page";
    }

    @PostMapping("/upload_file")
    public String uploadFile(@RequestParam MultipartFile[] files,
                             @RequestParam String publicationName,
                             @RequestParam String author,
                             @RequestParam String theme,
                             @RequestParam String yearOfPublication,
                             @RequestParam String description,
                             @RequestParam(value = "isPublic", required=false) Boolean isPublic) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();
        boolean publicFlag = isPublic != null;
        for (MultipartFile file: files)
            fileStorageService.saveFile(file, authentication.getName(),
                    author, publicationName, theme, yearOfPublication, description, publicFlag);
        return "redirect:/";
    }
}
